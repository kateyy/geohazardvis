#include <gui/data_view/ResidualVerificationView.h>

#include <algorithm>
#include <cassert>
#include <functional>

#include <QBoxLayout>
#include <QProgressBar>
#include <QToolBar>
#include <QtConcurrent/QtConcurrent>

#include <vtkCellData.h>
#include <vtkImageData.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyle.h>

#include <threadingzeug/parallelfor.h>

#include <core/AbstractVisualizedData.h>
#include <core/DataSetHandler.h>
#include <core/types.h>
#include <core/color_mapping/ColorBarRepresentation.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/utility/DataExtent.h>
#include <core/utility/InterpolationHelper.h>
#include <core/utility/vtkCameraSynchronization.h>

#include <gui/DataMapping.h>
#include <gui/data_view/RendererImplementationResidual.h>
#include <gui/data_view/RenderViewStrategy2D.h>


namespace
{

vtkSmartPointer<vtkDataArray> projectToLineOfSight(vtkDataArray & vectors, vtkVector3d lineOfSight)
{
    assert(vectors.GetNumberOfComponents() == 3);

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


ResidualVerificationView::ResidualVerificationView(DataMapping & dataMapping, int index, QWidget * parent, Qt::WindowFlags flags)
    : AbstractRenderView(dataMapping, index, parent, flags)
    , m_inSARLineOfSight(0, 0, 1)
    , m_interpolationMode(InterpolationMode::observationToModel)
    , m_observationUnitDecimalExponent(0)
    , m_modelUnitDecimalExponent(0)
    , m_observationData(nullptr)
    , m_modelData(nullptr)
    , m_modelEventsDeferred(false)
    , m_implementation(nullptr)
    , m_updateWatcher(std::make_unique<QFutureWatcher<void>>())
    , m_destructorCalled(false)
{
    connect(m_updateWatcher.get(), &QFutureWatcher<void>::finished, this, &ResidualVerificationView::handleUpdateFinished);
    m_attributeNamesLocations[residualIndex].first = "Residual"; // TODO add GUI option?

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

    initialize();   // lazy initialize is not really needed for now

    updateTitle();
}

ResidualVerificationView::~ResidualVerificationView()
{
    {
        m_destructorCalled = true;
        waitForResidualUpdate();
    }

    QList<AbstractVisualizedData *> toDelete;

    for (auto & vis : m_visualizations)
    {
        if (!vis)
            continue;

        toDelete << vis.get();
    }

    emit beforeDeleteVisualizations(toDelete);

    if (m_residual)
    {
        dataMapping().removeDataObjects({ m_residual.get() });
        dataSetHandler().removeEventFilter({ m_residual.get() });
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

void ResidualVerificationView::update()
{
    updateResidualAsync();
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
    return implementation().selectedData();
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
    if (subViewIndex >= 0 && subViewIndex < static_cast<int>(numberOfSubViews()))
    {
        unsigned int specificSubViewIdx = static_cast<unsigned int>(subViewIndex);
        if (m_visualizations[specificSubViewIdx].get() != &vis)
            return;

        implementation().lookAtData(vis, index, indexType, specificSubViewIdx);
        return;
    }

    for (unsigned int i = 0; i < numberOfSubViews(); ++i)
    {
        if (m_visualizations[i].get() != &vis)
            continue;

        implementation().lookAtData(vis, index, indexType, i);
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

DataObject * ResidualVerificationView::observationData()
{
    return m_observationData;
}

DataObject * ResidualVerificationView::modelData()
{
    return m_modelData;
}

DataObject * ResidualVerificationView::residualData()
{
    return m_residual.get();
}

int ResidualVerificationView::observationUnitDecimalExponent() const
{
    return m_observationUnitDecimalExponent;
}

void ResidualVerificationView::setObservationUnitDecimalExponent(int exponent)
{
    if (m_observationUnitDecimalExponent == exponent)
        return;

    m_observationUnitDecimalExponent = exponent;

    emit unitDecimalExponentsChanged(m_observationUnitDecimalExponent, m_modelUnitDecimalExponent);
}

int ResidualVerificationView::modelUnitDecimalExponent() const
{
    return m_modelUnitDecimalExponent;
}

void ResidualVerificationView::setModelUnitDecimalExponent(int exponent)
{
    if (m_modelUnitDecimalExponent == exponent)
        return;

    m_modelUnitDecimalExponent = exponent;

    emit unitDecimalExponentsChanged(m_observationUnitDecimalExponent, m_modelUnitDecimalExponent);
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

    emit interpolationModeChanged(mode);
}

ResidualVerificationView::InterpolationMode ResidualVerificationView::interpolationMode() const
{
    return m_interpolationMode;
}

void ResidualVerificationView::waitForResidualUpdate()
{
    m_updateWatcher->waitForFinished();
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void ResidualVerificationView::setDataHelper(unsigned int subViewIndex, DataObject * dataObject, bool skipResidualUpdate)
{
    assert(subViewIndex != residualIndex);

    if (dataAt(subViewIndex) == dataObject)
        return;

    setDataInternal(subViewIndex, dataObject, nullptr);

    if (dataObject)
    {
        assert(dataObject->dataSet());
        auto & dataSet = *dataObject->dataSet();
        m_attributeNamesLocations[subViewIndex] = findDataSetAttributeName(dataSet, subViewIndex);
    }
    else
    {
        m_attributeNamesLocations[subViewIndex] = {};
    }

    if (!skipResidualUpdate)
    {
        updateResidualAsync();
    }
}

unsigned int ResidualVerificationView::numberOfSubViews() const
{
    return numberOfViews;
}

RendererImplementation & ResidualVerificationView::implementation() const
{
    assert(m_implementation);
    return *m_implementation;
}

void ResidualVerificationView::showEvent(QShowEvent * event)
{
    AbstractRenderView::showEvent(event);

    initialize();
}

void ResidualVerificationView::showDataObjectsImpl(const QList<DataObject *> & dataObjects,
    QList<DataObject *> & incompatibleObjects,
    unsigned int subViewIndex)
{
    if (subViewIndex == residualIndex)
    {
        qDebug() << "Manually settings a residual is not supported in the ResidualVerificationView.";
        incompatibleObjects = dataObjects;
        return;
    }

    if (dataObjects.size() > 1)
        qDebug() << "Multiple objects per sub-view not supported in the ResidualVerificationView.";

    for (int i = 1; i < dataObjects.size(); ++i)
        incompatibleObjects << dataObjects[i];

    auto dataObject = dataObjects.isEmpty() ? nullptr : dataObjects.first();

    setDataHelper(subViewIndex, dataObject);
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
            if (auto dataObject = dataAt(i))
                objects << dataObject;
        }

        return objects;
    }

    if (auto dataObject = dataAt(unsigned(subViewIndex)))
        return{ dataObject };

    return{};
}

void ResidualVerificationView::prepareDeleteDataImpl(const QList<DataObject *> & dataObjects)
{
    bool changed = false;

    if (dataObjects.contains(m_observationData))
    {
        setDataHelper(observationIndex, nullptr, true);
        changed = true;
    }
    if (dataObjects.contains(m_modelData))
    {
        setDataHelper(modelIndex, nullptr, true);
        changed = true;
    }

    if (!changed)
    {
        return;
    }

    updateResidualAsync();
    // make sure that all references to the deleted data are already cleared
    waitForResidualUpdate();
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
    implementation().setAxesVisibility(enabled);
}

void ResidualVerificationView::initialize()
{
    if (m_implementation)
        return;

    m_implementation = std::make_unique<RendererImplementationResidual>(*this);
    m_implementation->activate(qvtkWidget());

    m_cameraSync = std::make_unique<vtkCameraSynchronization>();
    for (unsigned int i = 0; i < numberOfSubViews(); ++i)
        m_cameraSync->add(m_implementation->renderer(i));

    connect(m_implementation.get(), &RendererImplementation::dataSelectionChanged, this, &ResidualVerificationView::updateGuiSelection);
}

void ResidualVerificationView::setDataInternal(unsigned int subViewIndex, DataObject * dataObject, std::unique_ptr<DataObject> ownedObject)
{
    initialize();
    
    assert(!dataObject || !ownedObject); // only one of them should be used in the interface
    auto newData = dataObject ? dataObject : ownedObject.get();

    if (subViewIndex != residualIndex)
    {
        setDataAt(subViewIndex, dataObject);
    }
    else // owned residual data object. Update DataSetHandler and GUI accordingly
    {
        m_oldResidualToDeleteAfterUpdate = std::move(m_residual);
        if (m_oldResidualToDeleteAfterUpdate)
        {
            dataMapping().removeDataObjects({ m_oldResidualToDeleteAfterUpdate.get() });
            dataSetHandler().removeExternalData({ m_oldResidualToDeleteAfterUpdate.get() });
        }

        m_residual = std::move(ownedObject);

        if (m_residual)
        {
            dataSetHandler().addExternalData({ m_residual.get() });
        }
    }

    auto & oldVis = m_visualizations[subViewIndex];

    if (oldVis)
    {
        assert((subViewIndex != residualIndex) || m_oldResidualToDeleteAfterUpdate);

        implementation().removeContent(oldVis.get(), subViewIndex);

        m_visToDeleteAfterUpdate.push_back(std::move(oldVis));
    }

    if (newData)
    {
        auto newVis = implementation().requestVisualization(*newData);
        if (!newVis)
        {
            return;
        }

        auto newVisPtr = newVis.get();
        m_visualizations[subViewIndex] = std::move(newVis);
        implementation().addContent(newVisPtr, subViewIndex);
    }
}

void ResidualVerificationView::updateResidualAsync()
{
    waitForResidualUpdate();    // prevent locking the mutex multiple times in the main thread 

    std::unique_lock<std::mutex> updateLock(m_updateMutex);

    if (m_destructorCalled)
    {
        return;
    }

    if (!m_observationData || !m_modelData)
    {
        // handle the lock over, keep it locked until all update steps are done
        m_updateMutexLock = std::move(updateLock);
        // delete obsoleted residual data, update the UI
        handleUpdateFinished();
        return;
    }

    m_progressBar->show();
    toolBar()->setEnabled(false);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    assert(!m_newResidual);

    // while updating the residual, we might add transformed vector data to the model data set
    // make sure that this modification on the DataObject internals does not conflict with GUI events
    if (m_modelData)
    {
        assert(!m_modelEventsDeferred);
        m_modelData->deferEvents();
        m_modelEventsDeferred = true;
    }

    m_updateMutexLock = std::move(updateLock);
    auto future = QtConcurrent::run(this, &ResidualVerificationView::updateResidual);
    m_updateWatcher->setFuture(future);
}

void ResidualVerificationView::handleUpdateFinished()
{
    std::unique_lock<std::mutex> lock = std::move(m_updateMutexLock);

    if (m_newResidual)
    {
        auto computedResidual = m_newResidual;
        m_newResidual = nullptr;

        const bool residualIsImage = nullptr != vtkImageData::SafeDownCast(computedResidual);
        assert(residualIsImage || vtkPolyData::SafeDownCast(computedResidual));


        const bool reuseResidualObject = m_residual && m_residual->dataSet()->IsA(computedResidual->GetClassName());

        vtkSmartPointer<vtkDataSet> targetDataSet;
        if (reuseResidualObject)
        {
            targetDataSet = m_residual->dataSet();
        }
        else // create new Residual data object
        {
            targetDataSet.TakeReference(computedResidual->NewInstance());
        }

        // When creating a new residual data object, member structures need to be updated.
        std::unique_ptr<DataObject> newlyCreatedResidual;
        // In every case, the new geometry + data needs to be copied into the current residual data object
        DataObject * upToDateResidual = nullptr;

        if (reuseResidualObject)
        {
            upToDateResidual = m_residual.get();
        }
        else
        {
            if (residualIsImage)
            {
                newlyCreatedResidual = std::make_unique<ImageDataObject>("Residual", static_cast<vtkImageData &>(*targetDataSet));
            }
            else
            {
                newlyCreatedResidual = std::make_unique<PolyDataObject>("Residual", static_cast<vtkPolyData &>(*targetDataSet));
            }
            upToDateResidual = newlyCreatedResidual.get();
        }


        ScopedEventDeferral residualDeferal(*upToDateResidual);

        targetDataSet->CopyStructure(computedResidual);
        targetDataSet->CopyAttributes(computedResidual);

        if (newlyCreatedResidual)
        {
            setDataInternal(residualIndex, nullptr, std::move(newlyCreatedResidual));
        }
    }
    else if (m_residual)
    {
        // just remove the old residual

        setDataInternal(residualIndex, nullptr, nullptr);
    }

    if (m_modelEventsDeferred)
    {
        assert(m_modelData);
        m_modelEventsDeferred = false;
        m_modelData->executeDeferredEvents();
    }

    updateGuiAfterDataChange();

    toolBar()->setEnabled(true);
    m_progressBar->hide();
}

void ResidualVerificationView::updateResidual()
{
    assert(!m_newResidual);

    assert(!m_updateMutex.try_lock());

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

    const vtkSmartPointer<vtkDataArray> observationData = useObservationCellData
        ? observationDataSet.GetCellData()->GetArray(observationAttributeName.toUtf8().data())
        : observationDataSet.GetPointData()->GetArray(observationAttributeName.toUtf8().data());

    const vtkSmartPointer<vtkDataArray> modelData = useModelCellData
        ? modelDataSet.GetCellData()->GetArray(modelAttributeName.toUtf8().data())
        : modelDataSet.GetPointData()->GetArray(modelAttributeName.toUtf8().data());


    if (!observationData)
    {
        qDebug() << "Could not find valid observation data for residual computation (" << observationAttributeName + ")";
        return;
    }

    if (!modelData)
    {
        qDebug() << "Could not find valid model data for residual computation (" << modelAttributeName + ")";
        return;
    }


    // project displacement vectors to the line of sight vector, if required
    // This also adds the projection result as scalars to the respective data set, so that it can be used 
    // in the visualization.
    auto getProjectedDisp = [this] (vtkDataArray & displacement, unsigned int dataIndex, vtkDataSet & dataSet) 
        -> vtkSmartPointer<vtkDataArray> {

        if (displacement.GetNumberOfComponents() == 1)
        {
            m_projectedAttributeNames[dataIndex] = "";
            return &displacement;   // is already projected
        }

        if (displacement.GetNumberOfComponents() != 3)
        {   // can't handle that
            return nullptr;
        }

        auto projected = projectToLineOfSight(displacement, m_inSARLineOfSight);

        auto projectedName = m_attributeNamesLocations[dataIndex].first + " (projected)";
        m_projectedAttributeNames[dataIndex] = projectedName;
        projected->SetName(projectedName.toUtf8().data());

        if (m_attributeNamesLocations[dataIndex].second)    // use cell data?
            dataSet.GetCellData()->AddArray(projected);
        else
            dataSet.GetPointData()->AddArray(projected);

        return projected;
    };


    auto observationLosDisp = getProjectedDisp(*observationData, observationIndex, observationDataSet);
    auto modelLosDisp = getProjectedDisp(*modelData, modelIndex, modelDataSet);
    assert(observationLosDisp && modelLosDisp);


    // now interpolate one of the data arrays to the other's structure

    if (m_interpolationMode == InterpolationMode::modelToObservation)
    {
        auto attributeName = QString::fromUtf8(modelLosDisp->GetName());
        modelLosDisp = InterpolationHelper::interpolate(observationDataSet, modelDataSet, attributeName, useModelCellData);
    }
    else
    {
        auto attributeName = QString::fromUtf8(observationLosDisp->GetName());
        observationLosDisp = InterpolationHelper::interpolate(modelDataSet, observationDataSet, attributeName, useObservationCellData);
    }

    if (!observationLosDisp || !modelLosDisp)
    {
        qDebug() << "Observation/Model interpolation failed";

        return;
    }


    // expect line of sight displacements, matching to one of the structures here
    assert(modelLosDisp->GetNumberOfComponents() == 1);
    assert(observationLosDisp->GetNumberOfComponents() == 1);
    assert(modelLosDisp->GetNumberOfTuples() == observationLosDisp->GetNumberOfTuples());


    // compute the residual data

    auto & referenceDataSet = m_interpolationMode == InterpolationMode::modelToObservation
        ? observationDataSet
        : modelDataSet;

    auto residualData = vtkSmartPointer<vtkDataArray>::Take(modelLosDisp->NewInstance());
    residualData->SetName(m_attributeNamesLocations[residualIndex].first.toUtf8().data());
    residualData->SetNumberOfTuples(modelLosDisp->GetNumberOfTuples());
    residualData->SetNumberOfComponents(1);

    const double observationUnitFactor = std::pow(10, m_observationUnitDecimalExponent);
    const double modelUnitFactor = std::pow(10, m_modelUnitDecimalExponent);

    for (vtkIdType i = 0; i < residualData->GetNumberOfTuples(); ++i)
    {
        double o_value = observationLosDisp->GetTuple(i)[0] * observationUnitFactor;
        double m_value = modelLosDisp->GetTuple(i)[0] * modelUnitFactor;

        double r_value = o_value - m_value;
        residualData->SetTuple(i, &r_value);
    }


    assert(!m_newResidual);
    m_newResidual = vtkSmartPointer<vtkDataSet>::Take(referenceDataSet.NewInstance());
    m_newResidual->CopyStructure(&referenceDataSet);

    if (auto residualImage = vtkImageData::SafeDownCast(m_newResidual))
    {
        assert(residualData->GetNumberOfTuples() == residualImage->GetNumberOfPoints());

        residualImage->GetPointData()->SetScalars(residualData);
    }
    else if (auto residualPoly = vtkPolyData::SafeDownCast(m_newResidual))
    {
        // assuming that we store attributes in polygonal data always per cell
        assert(residualPoly->GetNumberOfCells() == residualData->GetNumberOfTuples());

        residualPoly->GetCellData()->SetScalars(residualData);
    }
    else
    {
        qDebug() << "Residual creation failed";

        return;
    }
}

void ResidualVerificationView::updateGuiAfterDataChange()
{
    if (m_visToDeleteAfterUpdate.size())
    {
        QList<AbstractVisualizedData *> toDelete;
        for (const auto & vis : m_visToDeleteAfterUpdate)
        {
            toDelete << vis.get();
        }
        emit beforeDeleteVisualizations(toDelete);
    }

    implementation().renderViewContentsChanged();
    emit visualizationsChanged();

    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    m_visToDeleteAfterUpdate.clear();
    m_oldResidualToDeleteAfterUpdate.reset();

    for (unsigned int i = 0; i < numberOfSubViews(); ++i)
    {
        if (!dataAt(i) || !m_visualizations[i])
            continue;

        auto attributeName = m_projectedAttributeNames[i];
        if (attributeName.isEmpty())
            attributeName = m_attributeNamesLocations[i].first;

        m_visualizations[i]->colorMapping().setCurrentScalarsByName(attributeName);
        m_visualizations[i]->colorMapping().setEnabled(true);
        m_visualizations[i]->colorMapping().colorBarRepresentation().setVisible(true);
    }

    updateGuiSelection();

    QList<DataObject *> validInputData;
    if (m_observationData)
        validInputData << m_observationData;
    if (m_modelData)
        validInputData << m_modelData;
    m_implementation->strategy2D().setInputData(validInputData);

    if (!validInputData.isEmpty())
        implementation().resetCamera(true, 0);

    render();
}

void ResidualVerificationView::updateGuiSelection()
{
    updateTitle();

    auto selectedVis = implementation().selectedData();
    auto selection = selectedVis ? &selectedVis->dataObject() : static_cast<DataObject *>(nullptr);

    // it's more convenient to chose on of the contents as "current", even if none is directly selected
    if (!selection)
    {
        selection = dataAt(activeSubViewIndex());
    }
    if (!selection)
    {
        auto vis = std::find_if(m_visualizations.begin(), m_visualizations.end(),
            [] (const decltype(m_visualizations)::value_type & vis) { return vis != nullptr; });
        if (vis != m_visualizations.end())
        {
            selection = &(*vis)->dataObject();
        }
    }

    emit selectedDataChanged(this, selection);
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

bool ResidualVerificationView::setDataAt(unsigned int i, DataObject * dataObject)
{
    switch (i)
    {
    case observationIndex:
        if (m_observationData == dataObject)
            return false;
        m_observationData = dataObject;
        break;
    case modelIndex:
        if (m_modelData == dataObject)
            return false;
        m_modelData = dataObject;
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
            for (int i = 0; i < dataSet.GetPointData()->GetNumberOfArrays(); ++i)
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
            for (int i = 0; i < dataSet.GetCellData()->GetNumberOfArrays(); ++i)
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
                scalars = dataSet.GetCellData()->GetArray("U-");
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
