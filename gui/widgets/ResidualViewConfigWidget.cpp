#include "ResidualViewConfigWidget.h"
#include "ui_ResidualViewConfigWidget.h"

#include <type_traits>

#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>

#include <core/DataSetHandler.h>
#include <gui/data_view/ResidualVerificationView.h>


ResidualViewConfigWidget::ResidualViewConfigWidget(QWidget * parent)
    : QWidget(parent)
    , m_ui(std::make_unique<Ui_ResidualViewConfigWidget>())
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

    for (auto && c : m_viewConnects)
    {
        disconnect(c);
    }

    m_viewConnects.clear();

    setEnabled(m_currentView != nullptr);
    if (!m_currentView)
    {
        m_ui->observationCombo->clear();
        m_ui->modelCombo->clear();
        return;
    }

    auto boolToIMode = [] (bool checked)
    {
        return checked
            ? ResidualVerificationView::InterpolationMode::modelToObservation
            : ResidualVerificationView::InterpolationMode::observationToModel;
    };
    auto iModeToBool = [boolToIMode] (ResidualVerificationView::InterpolationMode mode)
    {
        return boolToIMode(true) == mode;
    };

    connect(&view->dataSetHandler(), &DataSetHandler::dataObjectsChanged, this, &ResidualViewConfigWidget::updateComboBoxes);

    m_ui->interpolationModeCheckBox->setChecked(iModeToBool(view->interpolationMode()));
    m_viewConnects << connect(m_ui->interpolationModeCheckBox, &QCheckBox::toggled, [view, boolToIMode] (bool checked) {
        view->setInterpolationMode(boolToIMode(checked));
    });
    m_viewConnects << connect(view, &ResidualVerificationView::interpolationModeChanged, [this, iModeToBool] (ResidualVerificationView::InterpolationMode mode) {
        m_ui->interpolationModeCheckBox->blockSignals(true);
        m_ui->interpolationModeCheckBox->setChecked(iModeToBool(mode));
        m_ui->interpolationModeCheckBox->blockSignals(false);
    });
    
    auto && los = view->inSARLineOfSight();

    using LosType = decltype(los);

    auto uiSetLos = [this] (LosType los) {
        m_ui->losX->setValue(los.GetX());
        m_ui->losY->setValue(los.GetY());
        m_ui->losZ->setValue(los.GetZ());
    };
    uiSetLos(los);

    auto viewSetLos = [this, view] () {
        LosType los = { m_ui->losX->value(), m_ui->losY->value(), m_ui->losZ->value() };

        if (view->inSARLineOfSight() == los)
        {
            return;
        }

        view->setInSARLineOfSight(los);
    };

    m_viewConnects << connect(m_ui->losX, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), viewSetLos);
    m_viewConnects << connect(m_ui->losY, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), viewSetLos);
    m_viewConnects << connect(m_ui->losZ, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), viewSetLos);

    m_viewConnects << connect(view, &ResidualVerificationView::lineOfSightChanged, uiSetLos);


    m_viewConnects << connect(m_ui->observationScale, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
        view, &ResidualVerificationView::setObservationUnitDecimalExponent);
    m_viewConnects << connect(m_ui->modelScale, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
        view, &ResidualVerificationView::setModelUnitDecimalExponent);

    m_viewConnects << connect(view, &ResidualVerificationView::unitDecimalExponentsChanged, [this] (int o, int m) {
        m_ui->observationScale->blockSignals(true);
        m_ui->modelScale->blockSignals(true);

        m_ui->observationScale->setValue(o);
        m_ui->modelScale->setValue(m);

        m_ui->observationScale->blockSignals(false);
        m_ui->modelScale->blockSignals(false);
    });

    m_viewConnects << connect(m_ui->updateButton, &QAbstractButton::pressed, view, &ResidualVerificationView::update);

    updateComboBoxes();
}

void ResidualViewConfigWidget::updateComboBoxes()
{
    m_ui->observationCombo->blockSignals(true);
    m_ui->modelCombo->blockSignals(true);

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

        static_assert(sizeof(qulonglong) >= sizeof(size_t), "");
        qulonglong ptrData = static_cast<qulonglong>(reinterpret_cast<size_t>((dataObject)));

        if (dynamic_cast<ImageDataObject *>(dataObject))
        {
            m_ui->observationCombo->addItem(dataObject->name(), ptrData);
            m_ui->modelCombo->addItem(dataObject->name(), ptrData);
            continue;
        }

        if (auto poly = dynamic_cast<PolyDataObject *>(dataObject))
        {
            if (!poly->is2p5D())
                continue;

            m_ui->observationCombo->addItem(dataObject->name(), ptrData);
            m_ui->modelCombo->addItem(dataObject->name(), ptrData);
        }
    }

    QString observationName = m_currentView->observationData() ? m_currentView->observationData()->name() : "";
    QString modelName = m_currentView->modelData() ? m_currentView->modelData()->name() : "";
    m_ui->observationCombo->setCurrentText(observationName);
    m_ui->modelCombo->setCurrentText(modelName);

    m_ui->observationCombo->blockSignals(false);
    m_ui->modelCombo->blockSignals(false);
}

void ResidualViewConfigWidget::updateObservationFromUi(int index)
{
    if (!m_currentView)
        return;

    auto dataObject = reinterpret_cast<DataObject *>(m_ui->observationCombo->itemData(index, Qt::UserRole).toULongLong());

    m_currentView->setObservationData(dataObject);
}

void ResidualViewConfigWidget::updateModelFromUi(int index)
{
    if (!m_currentView)
        return;

    auto dataObject = reinterpret_cast<DataObject *>(m_ui->modelCombo->itemData(index, Qt::UserRole).toULongLong());

    m_currentView->setModelData(dataObject);
}
