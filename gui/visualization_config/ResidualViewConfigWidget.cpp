#include "ResidualViewConfigWidget.h"
#include "ui_ResidualViewConfigWidget.h"

#include <type_traits>

#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>

#include <core/DataSetHandler.h>
#include <core/utility/qthelper.h>
#include <gui/data_view/ResidualVerificationView.h>


ResidualViewConfigWidget::ResidualViewConfigWidget(QWidget * parent)
    : QWidget(parent)
    , m_ui{ std::make_unique<Ui_ResidualViewConfigWidget>() }
{
    m_ui->setupUi(this);

    connect(m_ui->observationCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &ResidualViewConfigWidget::updateObservationFromUi);
    connect(m_ui->modelCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &ResidualViewConfigWidget::updateModelFromUi);
}

ResidualViewConfigWidget::~ResidualViewConfigWidget() = default;

void ResidualViewConfigWidget::setCurrentView(ResidualVerificationView * view)
{
    m_currentView = view;

    disconnectAll(m_viewConnects);

    setEnabled(m_currentView != nullptr);
    if (!m_currentView)
    {
        m_ui->observationCombo->clear();
        m_ui->modelCombo->clear();
        return;
    }

    auto indexToGeometrySource = [] (int index)
    {
        return index == 0
            ? ResidualVerificationView::InputData::observation
            : ResidualVerificationView::InputData::model;
    };
    auto geometrySourceToIndex = [indexToGeometrySource] (ResidualVerificationView::InputData source)
    {
        return indexToGeometrySource(0) == source ? 0 : 1;
    };

    connect(&view->dataSetHandler(), &DataSetHandler::dataObjectsChanged, this, &ResidualViewConfigWidget::updateComboBoxes);

    m_ui->residualGeomtryCombo->setCurrentIndex(geometrySourceToIndex(view->residualGeometrySource()));
    const auto comboIndexChanged = static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged);
    m_viewConnects.emplace_back(connect(m_ui->residualGeomtryCombo, comboIndexChanged,
    [view, indexToGeometrySource] (int index)
    {
        view->setResidualGeometrySource(indexToGeometrySource(index));
    }));
    m_viewConnects.emplace_back(connect(view, &ResidualVerificationView::residualGeometrySourceChanged,
    [this, geometrySourceToIndex] (ResidualVerificationView::InputData source)
    {
        QSignalBlocker signalBlocker(m_ui->residualGeomtryCombo);
        m_ui->residualGeomtryCombo->setCurrentIndex(geometrySourceToIndex(source));
    }));

    auto && los = view->deformationLineOfSight();

    using LosType = decltype(los);

    auto uiSetLos = [this] (LosType los)
    {
        m_ui->losX->setValue(los.GetX());
        m_ui->losY->setValue(los.GetY());
        m_ui->losZ->setValue(los.GetZ());
    };
    uiSetLos(los);

    auto viewSetLos = [this, view] ()
    {
        LosType los = { m_ui->losX->value(), m_ui->losY->value(), m_ui->losZ->value() };

        if (view->deformationLineOfSight() == los)
        {
            return;
        }

        view->setDeformationLineOfSight(los);
    };

    const auto qSpinBoxValueChanged = static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged);
    const auto qdSpinBoxValueChanged = static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);

    m_viewConnects.emplace_back(connect(m_ui->losX, qdSpinBoxValueChanged, viewSetLos));
    m_viewConnects.emplace_back(connect(m_ui->losY, qdSpinBoxValueChanged, viewSetLos));
    m_viewConnects.emplace_back(connect(m_ui->losZ, qdSpinBoxValueChanged, viewSetLos));

    m_viewConnects.emplace_back(connect(view, &ResidualVerificationView::lineOfSightChanged, uiSetLos));


    m_viewConnects.emplace_back(connect(m_ui->observationScale, qSpinBoxValueChanged,
        view, &ResidualVerificationView::setObservationUnitDecimalExponent));
    m_viewConnects.emplace_back(connect(m_ui->modelScale, qSpinBoxValueChanged,
        view, &ResidualVerificationView::setModelUnitDecimalExponent));

    m_viewConnects.emplace_back(connect(view, &ResidualVerificationView::unitDecimalExponentsChanged,
    [this] (int o, int m)
    {
        QSignalBlocker observationSignalBlocker(m_ui->observationScale);
        QSignalBlocker modelSignalBlocker(m_ui->modelScale);

        m_ui->observationScale->setValue(o);
        m_ui->modelScale->setValue(m);
    }));

    m_viewConnects.emplace_back(connect(m_ui->updateButton, &QAbstractButton::pressed, view, &ResidualVerificationView::update));

    updateComboBoxes();
}

void ResidualViewConfigWidget::updateComboBoxes()
{
    const QSignalBlocker observationSignalBlocker(m_ui->observationCombo);
    const QSignalBlocker modelSignalBlocker(m_ui->modelCombo);

    m_ui->observationCombo->clear();
    m_ui->modelCombo->clear();


    if (!m_currentView)
    {
        return;
    }

    m_ui->observationCombo->addItem("", 0u);
    m_ui->modelCombo->addItem("", 0u);

    for (auto * dataObject : m_currentView->dataSetHandler().dataSets())
    {
        if (dataObject == m_currentView->residualData())
        {
            continue;
        }

        if (![dataObject] ()
            {
                if (dynamic_cast<ImageDataObject *>(dataObject))
                {
                    return true;
                }

                if (auto genericPoly = dynamic_cast<GenericPolyDataObject *>(dataObject))
                {
                    if (auto poly = dynamic_cast<PolyDataObject *>(genericPoly))
                    {
                        return poly->is2p5D();
                    }
                    return true;
                }

                return false;
            }())
        {
            continue;
        }

        const auto ptrData = dataObjectPtrToVariant(dataObject);
        m_ui->observationCombo->addItem(dataObject->name(), ptrData);
        m_ui->modelCombo->addItem(dataObject->name(), ptrData);
    }

    const QString observationName = m_currentView->observationData() ? m_currentView->observationData()->name() : "";
    const QString modelName = m_currentView->modelData() ? m_currentView->modelData()->name() : "";
    m_ui->observationCombo->setCurrentText(observationName);
    m_ui->modelCombo->setCurrentText(modelName);
}

void ResidualViewConfigWidget::updateObservationFromUi(int index)
{
    if (!m_currentView)
    {
        return;
    }

    auto dataObject = variantToDataObjectPtr(m_ui->observationCombo->itemData(index));
    m_currentView->setObservationData(dataObject);
}

void ResidualViewConfigWidget::updateModelFromUi(int index)
{
    if (!m_currentView)
    {
        return;
    }

    auto dataObject = variantToDataObjectPtr(m_ui->modelCombo->itemData(index));
    m_currentView->setModelData(dataObject);
}
