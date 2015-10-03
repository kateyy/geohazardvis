#include <gui/data_view/ResidualVerificationView.h>

#include <cassert>
#include <functional>
#include <random>

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QProgressBar>
#include <QToolBar>
#include <QtConcurrent/QtConcurrent>

#include <QVTKWidget.h>
#include <vtkCellData.h>
#include <vtkImageData.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>

#include <threadingzeug/parallelfor.h>

#include <core/AbstractVisualizedData.h>
#include <core/DataSetHandler.h>
#include <core/types.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/utility/DataExtent.h>
#include <core/utility/InterpolationHelper.h>
#include <core/utility/vtkCameraSynchronization.h>

#include <gui/SelectionHandler.h>
#include <gui/data_view/RendererImplementationResidual.h>
#include <gui/data_view/RenderViewStrategy2D.h>
#include <gui/data_view/RenderViewStrategy3D.h>


namespace
{

vtkSmartPointer<vtkDataArray> projectToLineOfSight(vtkDataArray & vectors, vtkVector3d lineOfSight)
{
    auto output = vtkSmartPointer<vtkDataArray>::Take(vectors.NewInstance());
    output->SetNumberOfComponents(1);
    output->SetNumberOfTuples(vectors.GetNumberOfTuples());

    lineOfSight.Normalize();

    auto projection = [output, &vectors, &lineOfSight](int i) {
        double scalarProjection = vtkMath::Dot(vectors.GetTuple(i), lineOfSight.GetData());
        output->SetTuple(i, &scalarProjection);
    };

    threadingzeug::sequential_for(0, output->GetNumberOfTuples(), projection);

    return output;
}

}


ResidualVerificationView::ResidualVerificationView(int index, QWidget * parent, Qt::WindowFlags flags)
    : AbstractRenderView(index, parent, flags)
    , m_qvtkMain(nullptr)
    , m_observationCombo(nullptr)
    , m_modelCombo(nullptr)
    , m_inSARLineOfSight(0, 0, 1)
    , m_interpolationMode(InterpolationMode::observationToModel)
    , m_observationUnitFactor(1.0)
    , m_modelUnitFactor(1.0)
    , m_observationData(nullptr)
    , m_modelData(nullptr)
    , m_implementation(nullptr)
    , m_strategy(nullptr)
    , m_updateWatcher(std::make_unique<QFutureWatcher<void>>())
{
    connect(m_updateWatcher.get(), &QFutureWatcher<void>::finished, this, &ResidualVerificationView::handleUpdateFinished);
    m_attributeNamesLocations[residualIndex].first = "Residual"; // TODO add GUI option?

    auto layout = new QBoxLayout(QBoxLayout::Direction::TopToBottom);
    layout->setMargin(0);
    layout->setSpacing(0);

    m_qvtkMain = new QVTKWidget(this);
    m_qvtkMain->setMinimumSize(300, 300);
    layout->addWidget(m_qvtkMain);

    setLayout(layout);

    auto interpolationSwitch = new QCheckBox("Interpolate Model to Observation");
    interpolationSwitch->setChecked(m_interpolationMode == InterpolationMode::observationToModel);
    // TODO correctly link in both ways
    connect(interpolationSwitch, &QAbstractButton::toggled, [this](bool checked)
    {
        setInterpolationMode(checked
            ? InterpolationMode::observationToModel
            : InterpolationMode::modelToObservation);
    });
    toolBar()->addWidget(interpolationSwitch);

    m_observationCombo = new QComboBox();
    m_modelCombo = new QComboBox();
    toolBar()->addWidget(m_observationCombo);
    toolBar()->addWidget(m_modelCombo);

    for (int i = 0; i < 3; ++i)
    {
        auto losEdit = new QDoubleSpinBox();
        losEdit->setRange(0, 1);
        losEdit->setSingleStep(0.01);
        losEdit->setValue(m_inSARLineOfSight[i]);
        connect(losEdit, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this, i, losEdit] (double val) {
            m_inSARLineOfSight[i] = val;
            updateResidualAsync();
        });
        connect(this, &ResidualVerificationView::lineOfSightChanged, [losEdit, i] (const vtkVector3d & los) {
            losEdit->setValue(los[i]);
        });
        toolBar()->addWidget(losEdit);
    }

    auto updateButton = new QPushButton("Update");
    connect(updateButton, &QAbstractButton::clicked, this, &ResidualVerificationView::updateResidualAsync);
    toolBar()->addWidget(updateButton);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 0);
    m_progressBar->setValue(-1);

    auto progressBarContainer = new QWidget();
    progressBarContainer->setMaximumWidth(100);
    auto pbcLayout = new QHBoxLayout();
    progressBarContainer->setLayout(pbcLayout);
    pbcLayout->setContentsMargins(0, 0, 0, 0);
    pbcLayout->addWidget(m_progressBar);
    m_progressBar->hide();
    toolBar()->addWidget(progressBarContainer);

    initialize();   // lazy initialize in not really needed for now

    updateTitle();

    SelectionHandler::instance().addRenderView(this);

    connect(&DataSetHandler::instance(), &DataSetHandler::dataObjectsChanged, this, &ResidualVerificationView::updateComboBoxes);

    connect(m_observationCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &ResidualVerificationView::updateObservationFromUi);
    connect(m_modelCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &ResidualVerificationView::updateModelFromUi);

    updateComboBoxes();
}

ResidualVerificationView::~ResidualVerificationView()
{
    SelectionHandler::instance().removeRenderView(this);

    for (auto & vis : m_visualizations)
    {
        if (!vis)
            continue;

        emit beforeDeleteVisualization(vis.get());
        vis.reset();
    }
}

QString ResidualVerificationView::friendlyName() const
{
    return QString::number(index()) + ": Residual Verification View";
}

QString ResidualVerificationView::subViewFriendlyName(unsigned int subViewIndex) const
{
    QString name;
    switch (subViewIndex)
    {
    case 0u:
        name = "Observation";
        break;
    case 1u:
        name = "Model";
        break; 
    case 2u:
        name = "Residual";
        break;
    }

    return name;
}

ContentType ResidualVerificationView::contentType() const
{
    return ContentType::Rendered2D;
}

DataObject * ResidualVerificationView::selectedData() const
{
    if (auto vis = selectedDataVisualization())
    {
        return &vis->dataObject();
    }

    return nullptr;
}

AbstractVisualizedData * ResidualVerificationView::selectedDataVisualization() const
{
    return m_implementation->selectedData();
}

void ResidualVerificationView::lookAtData(DataObject & dataObject, vtkIdType index, IndexType indexType, int subViewIndex)
{
    if (subViewIndex >= 0 && subViewIndex < static_cast<int>(numberOfSubViews()))
    {
        unsigned int specificSubViewIdx = static_cast<unsigned int>(subViewIndex);

        lookAtData(*m_visualizations[specificSubViewIdx].get(), index, indexType, subViewIndex);

        return;
    }

    for (unsigned int i = 0; i < numberOfSubViews(); ++i)
    {
        auto & vis = *m_visualizations[i].get();
        if (&vis.dataObject() != &dataObject)
            continue;

        lookAtData(vis, index, indexType, static_cast<int>(i));
    }
}

void ResidualVerificationView::lookAtData(AbstractVisualizedData & vis, vtkIdType index, IndexType indexType, int subViewIndex)
{
    unsigned int specificSubViewIdx = 0;

    if (subViewIndex >= 0 && subViewIndex < static_cast<int>(numberOfSubViews()))
    {
        specificSubViewIdx = static_cast<unsigned int>(subViewIndex);
        if (m_visualizations[specificSubViewIdx].get() != &vis)
            return;

        m_implementation->lookAtData(vis, index, indexType, specificSubViewIdx);
        return;
    }

    for (unsigned int i = 0; i < numberOfSubViews(); ++i)
    {
        if (m_visualizations[i].get() != &vis)
            continue;

        m_implementation->lookAtData(vis, index, indexType, i);
        return;
    }
}

AbstractVisualizedData * ResidualVerificationView::visualizationFor(DataObject * dataObject, int subViewIndex) const
{
    if (subViewIndex == -1)
    {
        for (unsigned int i = 0; i < numberOfSubViews(); ++i)
        {
            if (dataAt(i) == dataObject)
                return m_visualizations[i].get();
        }
        return nullptr;
    }

    assert(subViewIndex >= 0 && subViewIndex < 3);

    bool validAccess = dataAt(unsigned(subViewIndex)) == dataObject;
    if (!validAccess)
        return nullptr;

    return m_visualizations[subViewIndex].get();
}

int ResidualVerificationView::subViewContaining(const AbstractVisualizedData & visualizedData) const
{
    for (unsigned int i = 0u; i < numberOfSubViews(); ++i)
    {
        if (m_visualizations[i].get() == &visualizedData)
            return static_cast<int>(i);
    }
    
    return -1;
}

void ResidualVerificationView::setObservationData(DataObject * observation)
{
    setDataHelper(observationIndex, observation);
}

void ResidualVerificationView::setModelData(DataObject * model)
{
    setDataHelper(modelIndex, model);
}

void ResidualVerificationView::setResidualData(DataObject * residual)
{
    setDataHelper(residualIndex, residual);
}

void ResidualVerificationView::setInSARLineOfSight(const vtkVector3d & los)
{
    m_inSARLineOfSight = los;

    emit lineOfSightChanged(los);
}

const vtkVector3d & ResidualVerificationView::inSARLineOfSight() const
{
    return m_inSARLineOfSight;
}

void ResidualVerificationView::setInterpolationMode(InterpolationMode mode)
{
    if (m_interpolationMode == mode)
        return;

    m_interpolationMode = mode;

    updateResidualAsync();
}

ResidualVerificationView::InterpolationMode ResidualVerificationView::interpolationMode() const
{
    return m_interpolationMode;
}

void ResidualVerificationView::setDataHelper(unsigned int subViewIndex, DataObject * data, bool skipGuiUpdate, std::vector<std::unique_ptr<AbstractVisualizedData>> * toDelete)
{
    assert(skipGuiUpdate == (toDelete != nullptr));

    if (dataAt(subViewIndex) == data)
        return;

    std::vector<std::unique_ptr<AbstractVisualizedData>> toDeleteInternal;
    setDataInternal(subViewIndex, data, toDeleteInternal);

    if (data)
    {
        assert(data->dataSet());
        auto & dataSet = *data->dataSet();
        m_attributeNamesLocations[subViewIndex] = findDataSetAttributeName(dataSet, subViewIndex);
    }
    else
    {
        m_attributeNamesLocations[subViewIndex] = {};
    }

    if (subViewIndex != residualIndex)
        updateResidualAsync();

    // update GUI before actually deleting old visualization data

    if (skipGuiUpdate)
    {
        for (auto & it : toDeleteInternal)
            toDelete->push_back(std::move(it));
    }
    else
    {
        updateGuiAfterDataChange();
    }
}

unsigned int ResidualVerificationView::numberOfSubViews() const
{
    return numberOfViews;
}

vtkRenderWindow * ResidualVerificationView::renderWindow()
{
    assert(m_qvtkMain);
    return m_qvtkMain->GetRenderWindow();
}

const vtkRenderWindow * ResidualVerificationView::renderWindow() const
{
    assert(m_qvtkMain);
    return m_qvtkMain->GetRenderWindow();
}

RendererImplementation & ResidualVerificationView::implementation() const
{
    assert(m_implementation);
    return *m_implementation;
}

void ResidualVerificationView::showEvent(QShowEvent * event)
{
    AbstractDataView::showEvent(event);

    initialize();
}

QWidget * ResidualVerificationView::contentWidget()
{
    return m_qvtkMain;
}

void ResidualVerificationView::showDataObjectsImpl(const QList<DataObject *> & dataObjects,
    QList<DataObject *> & incompatibleObjects,
    unsigned int subViewIndex)
{
    if (dataObjects.size() > 1)
        qDebug() << "Multiple objects per sub-view not supported in the ResidualVerificationView.";

    for (int i = 1; i < dataObjects.size(); ++i)
        incompatibleObjects << dataObjects[i];

    DataObject * data = dataObjects.isEmpty() ? nullptr : dataObjects.first();

    setDataHelper(subViewIndex, data);
}

void ResidualVerificationView::hideDataObjectsImpl(const QList<DataObject *> & dataObjects, unsigned int subViewIndex)
{
    // no caching for now, just remove the object
    bool relevantRequest = dataObjects.contains(dataAt(subViewIndex));

    if (relevantRequest)
        setDataHelper(subViewIndex, nullptr);
}

QList<DataObject *> ResidualVerificationView::dataObjectsImpl(int subViewIndex) const
{
    if (subViewIndex == -1)
    {
        QList<DataObject *> objects;
        for (unsigned i = 0; i < numberOfSubViews(); ++i)
        {
            if (auto data = dataAt(i))
                objects << data;
        }

        return objects;
    }

    if (auto data = dataAt(unsigned(subViewIndex)))
        return{ data };

    return{};
}

void ResidualVerificationView::prepareDeleteDataImpl(const QList<DataObject *> & dataObjects)
{
    std::vector<std::unique_ptr<AbstractVisualizedData>> toDelete;

    if (dataObjects.contains(m_observationData))
        setDataHelper(observationIndex, nullptr, true, &toDelete);
    if (dataObjects.contains(m_modelData))
        setDataHelper(modelIndex, nullptr, true, &toDelete);
    assert(!dataObjects.contains(m_residual.get()));

    updateResidualAsync();

    updateGuiAfterDataChange();
}

QList<AbstractVisualizedData *> ResidualVerificationView::visualizationsImpl(int subViewIndex) const
{
    QList<AbstractVisualizedData *> validVis;

    if (subViewIndex == -1)
    {
        for (auto & vis : m_visualizations)
            if (vis)
                validVis << vis.get();
        return validVis;
    }

    if (m_visualizations[subViewIndex])
        return{ m_visualizations[subViewIndex].get() };

    return{};
}

void ResidualVerificationView::axesEnabledChangedEvent(bool enabled)
{
    m_implementation->setAxesVisibility(enabled);
}

void ResidualVerificationView::initialize()
{
    if (m_implementation)
        return;

    m_implementation = std::make_unique<RendererImplementationResidual>(*this);
    m_implementation->activate(m_qvtkMain);

    m_strategy = std::make_unique<RenderViewStrategy2D>(*m_implementation);

    m_implementation->setStrategy(m_strategy.get());

    m_cameraSync = std::make_unique<vtkCameraSynchronization>();
    for (unsigned int i = 0; i < numberOfSubViews(); ++i)
        m_cameraSync->add(m_implementation->renderer(i));
}

void ResidualVerificationView::setDataInternal(unsigned int subViewIndex, DataObject * dataObject, std::vector<std::unique_ptr<AbstractVisualizedData>> & toDelete)
{
    initialize();

    if (subViewIndex != residualIndex)
    {
        setDataAt(subViewIndex, dataObject);
    }
    else
    {
        // std::move for the residual cannot be done here
        assert(dataAt(subViewIndex) == dataObject);
    }

    auto & oldVis = m_visualizations[subViewIndex];

    if (oldVis)
    {
        m_implementation->removeContent(oldVis.get(), subViewIndex);

        beforeDeleteVisualization(oldVis.get());
        toDelete.push_back(std::move(oldVis));
    }

    if (dataObject)
    {
        auto newVis = m_implementation->requestVisualization(*dataObject);
        if (!newVis)
        {
            return;
        }
        auto newVisPtr = newVis.get();
        m_visualizations[subViewIndex] = std::move(newVis);
        m_implementation->addContent(newVisPtr, subViewIndex);
    }
}

void ResidualVerificationView::updateResidualAsync()
{
    m_updateWatcher->waitForFinished();

    m_progressBar->show();
    toolBar()->setEnabled(false);
    QCoreApplication::processEvents();

    assert(!m_newResidual);

    auto future = QtConcurrent::run(this, &ResidualVerificationView::updateResidual);
    m_updateWatcher->setFuture(future);
}

void ResidualVerificationView::handleUpdateFinished()
{
    auto oldResidual = std::move(m_residual);
    std::vector<std::unique_ptr<AbstractVisualizedData>> oldVisList;

    // TODO replace the double set
    setDataInternal(residualIndex, nullptr, oldVisList);
    assert(oldVisList.size() <= 1);

    std::unique_ptr<AbstractVisualizedData> oldVis;
    if (!oldVisList.empty())
    {
        oldVis = std::move(oldVisList.front());
        oldVisList.pop_back();
        assert(oldVisList.empty());
    }

    if (m_newResidual)
    {
        m_residual = std::move(m_newResidual);
        setDataInternal(residualIndex, m_residual.get(), oldVisList);
        assert(oldVisList.empty());
    }

    updateGuiAfterDataChange();

    toolBar()->setEnabled(true);
    m_progressBar->hide();
}

void ResidualVerificationView::updateResidual()
{
    assert(!m_newResidual);

    if (!m_observationData || !m_modelData)
    {
        return;
    }

    const auto & observationAttributeName = m_attributeNamesLocations[observationIndex].first;
    bool useObservationCellData = m_attributeNamesLocations[observationIndex].second;
    const auto & modelAttributeName = m_attributeNamesLocations[modelIndex].first;
    bool useModelCellData = m_attributeNamesLocations[modelIndex].second;


    if (observationAttributeName.isEmpty() || modelAttributeName.isEmpty())
    {
        qDebug() << "Cannot find suitable data attributes";

        return;
    }

    assert(m_observationData->dataSet());
    auto & observationDataSet = *m_observationData->dataSet();
    assert(m_modelData->dataSet());
    auto & modelDataSet = *m_modelData->dataSet();

    vtkSmartPointer<vtkDataArray> observationData;
    vtkSmartPointer<vtkDataArray> modelData;

    if (m_interpolationMode == InterpolationMode::modelToObservation)
    {
        observationData = observationDataSet.GetPointData()->GetArray(observationAttributeName.toUtf8().data());

        modelData = InterpolationHelper::interpolate(observationDataSet, modelDataSet, modelAttributeName, useModelCellData);
    }
    else
    {
        observationData = InterpolationHelper::interpolate(modelDataSet, observationDataSet, observationAttributeName, useObservationCellData);

        modelData = useModelCellData
            ? modelDataSet.GetCellData()->GetArray(modelAttributeName.toUtf8().data())
            : modelDataSet.GetPointData()->GetArray(modelAttributeName.toUtf8().data());
    }

    if (!observationData || !modelData)
    {
        qDebug() << "Observation/Model interpolation failed";

        return;
    }

    // project vectors if needed and assign arrays accordingly
    if (observationData->GetNumberOfComponents() == 3)
    {
        observationData = projectToLineOfSight(*observationData, m_inSARLineOfSight);

        m_projectedAttributeNames[observationIndex] = observationAttributeName + " (projected)";
        auto projectedName = m_projectedAttributeNames[observationIndex].toUtf8();
        observationData->SetName(projectedName.data());

        if (useObservationCellData)
            observationDataSet.GetCellData()->AddArray(observationData);
        else
            observationDataSet.GetPointData()->AddArray(observationData);
    }
    else
    {
        m_projectedAttributeNames[observationIndex] = "";
    }

    if (modelData->GetNumberOfComponents() == 3)
    {
        modelData = projectToLineOfSight(*modelData, m_inSARLineOfSight);

        m_projectedAttributeNames[modelIndex] = modelAttributeName + " (projected)";
        auto projectedName = m_projectedAttributeNames[modelIndex].toUtf8();
        modelData->SetName(projectedName.data());

        if (useModelCellData)
            modelDataSet.GetCellData()->AddArray(modelData);
        else
            modelDataSet.GetPointData()->AddArray(modelData);
    }
    else
    {
        m_projectedAttributeNames[modelIndex] = "";
    }

    assert(modelData->GetNumberOfComponents() == 1);
    assert(observationData->GetNumberOfComponents() == 1);
    assert(modelData->GetNumberOfTuples() == observationData->GetNumberOfTuples());


    // compute the residual data

    auto & referenceDataSet = m_interpolationMode == InterpolationMode::modelToObservation
        ? observationDataSet
        : modelDataSet;

    auto residualData = vtkSmartPointer<vtkDataArray>::Take(modelData->NewInstance());
    residualData->SetName(m_attributeNamesLocations[residualIndex].first.toUtf8().data());
    residualData->SetNumberOfTuples(modelData->GetNumberOfTuples());
    residualData->SetNumberOfComponents(1);

    for (vtkIdType i = 0; i < residualData->GetNumberOfTuples(); ++i)
    {
        auto o_value = observationData->GetTuple(i)[0] * m_observationUnitFactor;
        auto m_value = modelData->GetTuple(i)[0] * m_modelUnitFactor;

        double r_value = o_value - m_value;
        residualData->SetTuple(i, &r_value);
    }


    std::unique_ptr<DataObject> residualReplacement;

    auto residual = vtkSmartPointer<vtkDataSet>::Take(referenceDataSet.NewInstance());
    residual->CopyStructure(&referenceDataSet);

    // TODO do not recreate data DataObject if not needed
    if (auto residualImage = vtkImageData::SafeDownCast(residual))
    {
        assert(residualData->GetNumberOfTuples() == residual->GetNumberOfPoints());

        residual->GetPointData()->SetScalars(residualData);

        residualReplacement = std::make_unique<ImageDataObject>("Residual", *residualImage);
    }
    else if (auto residualPoly = vtkPolyData::SafeDownCast(residual))
    {
        assert(vtkPolyData::SafeDownCast(residual));
        // assuming that we store attributes in polygonal data always per cell
        assert(residual->GetNumberOfCells() == residualData->GetNumberOfTuples());

        residual->GetCellData()->SetScalars(residualData);

        residualReplacement = std::make_unique<PolyDataObject>("Residual", *residualPoly);
    }
    else
    {
        qDebug() << "Residual creation failed";

        return;
    }

    m_newResidual = std::move(residualReplacement);
}

void ResidualVerificationView::updateGuiAfterDataChange()
{
    emit visualizationsChanged();

    for (unsigned int i = 0; i < numberOfSubViews(); ++i)
    {
        if (!dataAt(i))
            continue;

        auto attributeName = m_projectedAttributeNames[i];
        if (attributeName.isEmpty())
            attributeName = m_attributeNamesLocations[i].first;

        m_implementation->colorMapping(i)->setCurrentScalarsByName(attributeName);
    }

    updateGuiSelection();

    QList<DataObject *> validInputData;
    if (m_observationData)
        validInputData << m_observationData;
    if (m_modelData)
        validInputData << m_modelData;
    m_strategy->setInputData(validInputData);

    if (!validInputData.isEmpty())
        implementation().resetCamera(true, 0);

    render();
}

void ResidualVerificationView::updateGuiSelection()
{
    updateTitle();

    DataObject * selection = nullptr;
    for (auto & vis : m_visualizations)
    {
        if (vis)
        {
            selection = &vis->dataObject();
            break;
        }
    }

    emit selectedDataChanged(this, selection);
}

void ResidualVerificationView::updateComboBoxes()
{
    m_observationCombo->blockSignals(true);
    m_modelCombo->blockSignals(true);


    QString oldObservationName = m_observationCombo->currentText();
    QString oldModelName = m_modelCombo->currentText();

    m_observationCombo->clear();
    m_modelCombo->clear();

    QList<ImageDataObject *> images;
    QList<PolyDataObject *> polyData2p5D;

    for (DataObject * data : DataSetHandler::instance().dataSets())
    {
        qulonglong ptrData = reinterpret_cast<size_t>(data);

        // ImageDataObjects can directly be used as observation/model
        if (dynamic_cast<ImageDataObject *>(data))
        {
            m_observationCombo->addItem(data->name(), ptrData);
            m_modelCombo->addItem(data->name(), ptrData);
            continue;
        }

        // allow to transform 2.5D polygonal data into model surface grid
        if (auto poly = dynamic_cast<PolyDataObject *>(data))
        {
            if (!poly->is2p5D())
                continue;

            m_modelCombo->addItem(data->name(), ptrData);
        }
    }

    m_observationCombo->setCurrentIndex(-1);
    for (int i = 0; i < m_observationCombo->count(); ++i)
    {
        if (m_observationCombo->itemText(i) != oldObservationName)
            continue;

        m_observationCombo->setCurrentIndex(i);
        break;
    }

    m_modelCombo->setCurrentIndex(-1);
    for (int i = 0; i < m_modelCombo->count(); ++i)
    {
        if (m_modelCombo->itemText(i) != oldModelName)
            continue;

        m_modelCombo->setCurrentIndex(i);
        break;
    }

    m_observationCombo->blockSignals(false);
    m_modelCombo->blockSignals(false);
}

void ResidualVerificationView::updateObservationFromUi(int index)
{
    auto data = reinterpret_cast<DataObject *>(m_observationCombo->itemData(index, Qt::UserRole).toULongLong());

    setObservationData(data);
}

void ResidualVerificationView::updateModelFromUi(int index)
{
    auto data = reinterpret_cast<DataObject *>(m_modelCombo->itemData(index, Qt::UserRole).toULongLong());

    setModelData(data);
}

DataObject * ResidualVerificationView::dataAt(unsigned int i) const
{
    switch (i)
    {
    case observationIndex:
        return m_observationData;
    case modelIndex:
        return m_modelData;
    case residualIndex:
        return m_residual.get();
    }
    assert(false);
    return nullptr;
}

bool ResidualVerificationView::setDataAt(unsigned int i, DataObject * data)
{
    switch (i)
    {
    case observationIndex:
        if (m_observationData == data)
            return false;
        m_observationData = data;
        break;
    case modelIndex:
        if (m_modelData == data)
            return false;
        m_modelData = data;
        break;
    default:
        assert(false);
        return false;
    }
    return true;
}

std::pair<QString, bool> ResidualVerificationView::findDataSetAttributeName(vtkDataSet & dataSet, unsigned int inputType)
{
    auto checkDefaultScalars = [] (vtkDataSet & dataSet) -> std::pair<QString, bool>
    {
        if (auto scalars = dataSet.GetPointData()->GetScalars())
        {
            if (auto name = scalars->GetName())
                return std::make_pair(QString::fromUtf8(name), false);
        }
        else
        {
            if (auto firstArray = dataSet.GetPointData()->GetArray(0))
            {
                if (auto name = firstArray->GetName())
                    return std::make_pair(QString::fromUtf8(name), false);
            }
        }

        if (auto scalars = dataSet.GetCellData()->GetScalars())
        {
            if (auto name = scalars->GetName())
                return std::make_pair(QString::fromUtf8(name), true);
        }
        else
        {
            if (auto firstArray = dataSet.GetCellData()->GetArray(0))
            {
                if (auto name = firstArray->GetName())
                    return std::make_pair(QString::fromUtf8(name), true);
            }
        }
        return{};
    };

    auto checkDefaultVectors = [] (vtkDataSet & dataSet) -> std::pair<QString, bool>
    {
        if (auto scalars = dataSet.GetPointData()->GetVectors())
        {
            if (auto name = scalars->GetName())
                return std::make_pair(QString::fromUtf8(name), false);
        }
        else
        {
            for (vtkIdType i = 0; i < dataSet.GetPointData()->GetNumberOfArrays(); ++i)
            {
                auto array = dataSet.GetPointData()->GetArray(i);
                if (array->GetNumberOfComponents() != 3)
                    continue;

                if (auto name = array->GetName())
                    return std::make_pair(QString::fromUtf8(name), false);
            }
        }

        if (auto scalars = dataSet.GetCellData()->GetVectors())
        {
            if (auto name = scalars->GetName())
                return std::make_pair(QString::fromUtf8(name), true);
        }
        else
        {
            for (vtkIdType i = 0; i < dataSet.GetCellData()->GetNumberOfArrays(); ++i)
            {
                auto array = dataSet.GetCellData()->GetArray(i);
                if (array->GetNumberOfComponents() != 3)
                    continue;

                if (auto name = array->GetName())
                    return std::make_pair(QString::fromUtf8(name), false);
            }
        }
        return{};
    };

    bool isPoly = vtkPolyData::SafeDownCast(&dataSet) != nullptr;

    if (inputType == observationIndex)
    {
        return checkDefaultScalars(dataSet);
    }

    if (inputType == modelIndex)
    {
        if (isPoly)
        {
            // assuming that we store attributes in polygonal data always per cell
            auto scalars = dataSet.GetCellData()->GetScalars();
            if (!scalars)
                scalars = dataSet.GetCellData()->GetArray("displacement vectors");
            if (!scalars)
                scalars = dataSet.GetCellData()->GetArray("U+");
            if (scalars)
            {
                auto name = scalars->GetName();
                if (name)
                    return std::make_pair(QString::fromUtf8(name), true);
            }
        }

        auto result = checkDefaultVectors(dataSet);
        if (!result.first.isEmpty())
            return result;

        return checkDefaultScalars(dataSet);
    }

    return std::make_pair(QString("Residual"), true);
}
