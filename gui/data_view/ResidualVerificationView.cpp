#include <gui/data_view/ResidualVerificationView.h>

#include <algorithm>
#include <cassert>
#include <functional>
#include <type_traits>

#include <QBoxLayout>
#include <QProgressBar>
#include <QToolBar>
#include <QtConcurrent/QtConcurrent>

#include <vtkArrayDispatch.h>
#include <vtkAssume.h>
#include <vtkCellData.h>
#include <vtkDataArrayAccessor.h>
#include <vtkImageData.h>
#include <vtkInteractorStyle.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSMPTools.h>
#include <vtkVector.h>

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
#include <core/utility/vtkvectorhelper.h>

#include <gui/DataMapping.h>
#include <gui/data_view/RendererImplementationResidual.h>
#include <gui/data_view/RenderViewStrategy2D.h>

namespace
{

struct ProjectToLineOfSightWorker
{
    vtkVector3d lineOfSight;
    vtkSmartPointer<vtkDataArray> projectedData;

    template<typename Vector_t>
    void operator()(Vector_t * vectors)
    {
        VTK_ASSUME(vectors->GetNumberOfComponents() == 3);

        using ValueType = typename vtkDataArrayAccessor<Vector_t>::APIType;

        auto output = vtkSmartPointer<vtkAOSDataArrayTemplate<ValueType>>::New();
        output->SetNumberOfTuples(vectors->GetNumberOfTuples());

        lineOfSight.Normalize();
        const auto los = convertTo<ValueType>(lineOfSight);

        vtkDataArrayAccessor<Vector_t> v(vectors);
        vtkDataArrayAccessor<Vector_t> l(output);

        vtkSMPTools::For(0, vectors->GetNumberOfTuples(),
            [v, l, los] (vtkIdType begin, vtkIdType end)
        {
            vtkVector3<ValueType> vector;
            for (vtkIdType i = begin; i < end; ++i)
            {
                v.Get(i, vector.GetData());
                const auto projection = vector.Dot(los);
                l.Set(i, 0, projection);
            };
        });

        projectedData = output;
    }
};

struct ResidualWorker
{
    double observationUnitFactor;
    double modelUnitFactor;

    vtkSmartPointer<vtkDataArray> residual;

    template<typename Observation_t, typename Model_t>
    void operator()(Observation_t * observation, Model_t * model)
    {
        VTK_ASSUME(observation->GetNumberOfComponents() == 1);
        VTK_ASSUME(model->GetNumberOfComponents() == 1);
        VTK_ASSUME(model->GetNumberOfTuples() == observation->GetNumberOfTuples());

        using ObservationValue_t = typename vtkDataArrayAccessor<Observation_t>::APIType;
        using ModelValue_t = typename vtkDataArrayAccessor<Model_t>::APIType;
        using ResidualValue_t = std::common_type_t<ObservationValue_t, ModelValue_t>;

        auto res = vtkSmartPointer<vtkAOSDataArrayTemplate<ResidualValue_t>>::New();
        res->SetNumberOfValues(observation->GetNumberOfTuples());

        vtkDataArrayAccessor<Observation_t> o(observation);
        vtkDataArrayAccessor<Model_t> m(model);
        vtkDataArrayAccessor<vtkAOSDataArrayTemplate<ResidualValue_t>> r(res);

        const auto oUnit = static_cast<ObservationValue_t>(observationUnitFactor);
        const auto mUnit = static_cast<ModelValue_t>(modelUnitFactor);

        vtkSMPTools::For(0, res->GetNumberOfValues(),
            [o, m, r, oUnit, mUnit] (vtkIdType begin, vtkIdType end)
        {
            for (vtkIdType i = begin; i < end; ++i)
            {
                const auto diff = static_cast<ResidualValue_t>(
                    o.Get(i, 0) * oUnit
                    - m.Get(i, 0) * mUnit);
                r.Set(i, 0, diff);
            }
        });

        residual = res;
    }
};

}


ResidualVerificationView::ResidualVerificationView(DataMapping & dataMapping, int index, QWidget * parent, Qt::WindowFlags flags)
    : AbstractRenderView(dataMapping, index, parent, flags)
    , m_inSARLineOfSight{ 0, 0, 1 }
    , m_interpolationMode{ InterpolationMode::observationToModel }
    , m_observationUnitDecimalExponent{ 0 }
    , m_modelUnitDecimalExponent{ 0 }
    , m_observationData{ nullptr }
    , m_modelData{ nullptr }
    , m_modelEventsDeferred{ false }
    , m_implementation{ nullptr }
    , m_updateWatcher{ std::make_unique<QFutureWatcher<void>>() }
    , m_destructorCalled{ false }
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
    signalClosing();

    {
        m_destructorCalled = true;
        waitForResidualUpdate();
    }

    QList<AbstractVisualizedData *> toDelete;

    for (auto & vis : m_visualizations)
    {
        if (!vis)
        {
            continue;
        }

        toDelete << vis.get();
    }

    emit beforeDeleteVisualizations(toDelete);

    if (m_residual)
    {
        dataMapping().removeDataObjects({ m_residual.get() });
        dataSetHandler().removeExternalData({ m_residual.get() });
    }
}

void ResidualVerificationView::update()
{
    updateResidualAsync();
}

ContentType ResidualVerificationView::contentType() const
{
    return ContentType::Rendered2D;
}

void ResidualVerificationView::lookAtData(const DataSelection & selection, int subViewIndex)
{
    if (!selection.dataObject)
    {
        return;
    }

    if (subViewIndex >= 0 && subViewIndex < static_cast<int>(numberOfSubViews()))
    {
        const auto specificSubViewIdx = static_cast<unsigned int>(subViewIndex);

        auto vis = m_visualizations[specificSubViewIdx].get();
        if (!vis)
        {
            return;
        }

        lookAtData(VisualizationSelection(selection,
            vis,
            0),
            subViewIndex);

        return;
    }

    for (unsigned int i = 0; i < numberOfSubViews(); ++i)
    {
        auto * vis = m_visualizations[i].get();
        if (&vis->dataObject() != selection.dataObject)
        {
            continue;
        }

        lookAtData(VisualizationSelection(selection,
            vis,
            0),
            subViewIndex);
    }
}

void ResidualVerificationView::lookAtData(const VisualizationSelection & selection, int subViewIndex)
{
    if (subViewIndex >= 0 && subViewIndex < static_cast<int>(numberOfSubViews()))
    {
        unsigned int specificSubViewIdx = static_cast<unsigned int>(subViewIndex);
        if (m_visualizations[specificSubViewIdx].get() != selection.visualization)
        {
            return;
        }

        implementation().lookAtData(selection, specificSubViewIdx);
        return;
    }

    for (unsigned int i = 0; i < numberOfSubViews(); ++i)
    {
        if (m_visualizations[i].get() != selection.visualization)
        {
            continue;
        }

        implementation().lookAtData(selection, i);
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
            {
                return m_visualizations[i].get();
            }
        }
        return nullptr;
    }

    assert(subViewIndex >= 0 && subViewIndex < 3);

    if (dataAt(unsigned(subViewIndex)) != dataObject)
    {
        return nullptr;
    }

    return m_visualizations[subViewIndex].get();
}

int ResidualVerificationView::subViewContaining(const AbstractVisualizedData & visualizedData) const
{
    for (unsigned int i = 0u; i < numberOfSubViews(); ++i)
    {
        if (m_visualizations[i].get() == &visualizedData)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

bool ResidualVerificationView::isEmpty() const
{
    return !m_observationData && !m_modelData;
}

void ResidualVerificationView::setObservationData(DataObject * observation)
{
    waitForResidualUpdate();

    setDataHelper(observationIndex, observation);
}

void ResidualVerificationView::setModelData(DataObject * model)
{
    waitForResidualUpdate();

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
    {
        return;
    }

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
    {
        return;
    }

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
    {
        return;
    }

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
    if (!m_updateWatcher->isStarted())
    {
        return;
    }

    m_updateWatcher->waitForFinished();
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void ResidualVerificationView::setDataHelper(unsigned int subViewIndex, DataObject * dataObject, bool skipResidualUpdate)
{
    assert(subViewIndex != residualIndex);

    if (dataAt(subViewIndex) == dataObject)
    {
        return;
    }

    setDataInternal(subViewIndex, dataObject, nullptr);

    vtkSmartPointer<vtkDataSet> dataSet = dataObject ? dataObject->dataSet() : nullptr;
    if (dataObject && !dataSet)
    {
        dataSet = dataObject->processedOutputDataSet();
        if (!dataSet)
        {
            qDebug() << "Unsupported data object (no data found):" << dataObject->name();
        }
    }
    if (dataObject && dataSet)
    {
        m_attributeNamesLocations[subViewIndex] = findDataSetAttributeName(*dataSet, subViewIndex);
    }
    else
    {
        m_attributeNamesLocations[subViewIndex] = std::pair<QString, bool>{};
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

void ResidualVerificationView::initializeRenderContext()
{
    initialize();
}

std::pair<QString, std::vector<QString>> ResidualVerificationView::friendlyNameInternal() const
{
    return{
        QString::number(index()) + ": Residual Verification View",
        { "Observation", "Model", "Residual" }
    };
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
    {
        qDebug() << "Multiple objects per sub-view not supported in the ResidualVerificationView.";
    }

    for (int i = 1; i < dataObjects.size(); ++i)
    {
        incompatibleObjects << dataObjects[i];
    }

    auto dataObject = dataObjects.isEmpty() ? nullptr : dataObjects.first();

    setDataHelper(subViewIndex, dataObject);
}

void ResidualVerificationView::hideDataObjectsImpl(const QList<DataObject *> & dataObjects, int subViewIndex)
{
    // check if this request is relevant
    if (subViewIndex < 0)
    {
        if (dataObjects.toSet().intersect({ m_observationData, m_modelData }).isEmpty())
        {
            return;
        }
    }
    else if (!dataObjects.contains(dataAt(static_cast<unsigned>(subViewIndex))))
    {
        return;
    }

    waitForResidualUpdate();

    // no caching for now, just remove the object

    if (subViewIndex >= 0)
    {
        setDataHelper(static_cast<unsigned>(subViewIndex), nullptr, true);
    }
    else
    {
        for (unsigned i = 0; i < numberOfViews; ++i)
        {
            if (dataObjects.contains(dataAt(i)))
            {
                setDataHelper(i, nullptr, true);
            }
        }
    }

    updateResidualAsync();

    waitForResidualUpdate();
}

QList<DataObject *> ResidualVerificationView::dataObjectsImpl(int subViewIndex) const
{
    if (subViewIndex == -1)
    {
        QList<DataObject *> objects;
        for (unsigned i = 0; i < numberOfSubViews(); ++i)
        {
            if (auto dataObject = dataAt(i))
            {
                objects << dataObject;
            }
        }

        return objects;
    }

    if (auto dataObject = dataAt(unsigned(subViewIndex)))
    {
        return{ dataObject };
    }

    return{};
}

void ResidualVerificationView::prepareDeleteDataImpl(const QList<DataObject *> & dataObjects)
{
    const bool unsetObservation = dataObjects.contains(m_observationData);
    const bool unsetModel = dataObjects.contains(m_modelData);

    if (!unsetObservation && !unsetModel)
    {
        return;
    }

    // don't change internal data if the update process is currently running
    waitForResidualUpdate();

    if (unsetObservation)
    {
        setDataHelper(observationIndex, nullptr, true);
    }
    if (unsetModel)
    {
        setDataHelper(modelIndex, nullptr, true);
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
        {
            if (vis)
            {
                validVis << vis.get();
            }
        }
        return validVis;
    }

    if (m_visualizations[subViewIndex])
    {
        return{ m_visualizations[subViewIndex].get() };
    }

    return{};
}

void ResidualVerificationView::axesEnabledChangedEvent(bool enabled)
{
    implementation().setAxesVisibility(enabled);
}

void ResidualVerificationView::initialize()
{
    if (m_implementation)
    {
        return;
    }

    m_implementation = std::make_unique<RendererImplementationResidual>(*this);
    m_implementation->activate(qvtkWidget());

    m_cameraSync = std::make_unique<vtkCameraSynchronization>();
    for (unsigned int i = 0; i < numberOfSubViews(); ++i)
    {
        m_cameraSync->add(m_implementation->renderer(i));
    }
}

void ResidualVerificationView::setDataInternal(unsigned int subViewIndex, DataObject * dataObject, std::unique_ptr<DataObject> ownedObject)
{
    assert(m_implementation);

    assert(!dataObject || !ownedObject); // only one of them should be used in the interface
    auto newData = dataObject ? dataObject : ownedObject.get();

    if (subViewIndex != residualIndex)
    {
        // Try to find a coordinate system that is supported by both data sets.
        auto newTransformable = dynamic_cast<CoordinateTransformableDataObject *>(newData);
        auto presentTransformable = dynamic_cast<CoordinateTransformableDataObject *>(
            subViewIndex == observationIndex ? dataAt(modelIndex) : dataAt(observationIndex));

        // Change the coordinate system only if it is not currently set or if the new data is not
        // compatible with the current system.
        // Meanwhile, make sure that already loaded data can also be shown in the new system.
        if (newTransformable
            && (!currentCoordinateSystem().isValid()
            || (currentCoordinateSystem().type == CoordinateSystemType::unspecified)
            || !newTransformable->canTransformTo(currentCoordinateSystem())))
        {
            auto targetCoords = [newTransformable, presentTransformable] ()
            {
                // Prefer local metric coordinates, fall back to global metric.
                // If even this does not work, use the data set's current system, or just don't
                // transform at all.

                auto spec = CoordinateSystemSpecification(newTransformable->coordinateSystem());
                spec.unitOfMeasurement = "km";
                spec.type = CoordinateSystemType::metricLocal;
                if (newTransformable->canTransformTo(spec)
                    && (!presentTransformable || presentTransformable->canTransformTo(spec)))
                {
                    return spec;
                }

                spec.type = CoordinateSystemType::metricGlobal;
                if (newTransformable->canTransformTo(spec)
                    && (!presentTransformable || presentTransformable->canTransformTo(spec)))
                {
                    return spec;
                }

                spec = newTransformable->coordinateSystem();
                if (spec.isValid()
                    && newTransformable->canTransformTo(spec)
                    && (!presentTransformable || presentTransformable->canTransformTo(spec)))
                {
                    return spec;
                }

                // No way :(
                spec = {};
                return spec;
            }();

            setCurrentCoordinateSystem(targetCoords);
        }

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
        auto transformableResidual = dynamic_cast<CoordinateTransformableDataObject *>(m_residual.get());
        auto transformableObservation = dynamic_cast<CoordinateTransformableDataObject *>(m_observationData);
        if (transformableResidual && transformableObservation)
        {
            // Apply reference point from the observation data, set current coordinate system type
            // to the current view system.
            auto spec = transformableObservation->coordinateSystem();
            auto && renderSpec = currentCoordinateSystem();
            spec.type = renderSpec.type;
            spec.unitOfMeasurement = renderSpec.unitOfMeasurement;
            transformableResidual->specifyCoordinateSystem(spec);
        }

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

    resetFriendlyName();
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

        {

            ScopedEventDeferral residualDeferal(*upToDateResidual);

            upToDateResidual->CopyStructure(*computedResidual);
            upToDateResidual->dataSet()->CopyAttributes(computedResidual);
        }

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
    const bool useObservationCellData = m_attributeNamesLocations[observationIndex].second;
    const auto & modelAttributeName = m_attributeNamesLocations[modelIndex].first;
    const bool useModelCellData = m_attributeNamesLocations[modelIndex].second;

    if (observationAttributeName.isEmpty() || modelAttributeName.isEmpty())
    {
        qDebug() << "ResidualVerificationView::updateResidual: Cannot find suitable data attributes";

        for (unsigned i = 0; i < numberOfViews; ++i)
        {
            if (m_attributeNamesLocations[i].first.isEmpty())
            {
                m_projectedAttributeNames = {};
            }
        }

        return;
    }

    if (!m_observationData->dataSet() || !m_modelData->dataSet())
    {
        // Projected attributes are added to the point/cell data of the DataObject::dataSet().
        // If the data object does not have such a source data set, where should the projected
        // data be added, so that it can be found by the color mapping?
        qDebug() << "Input data is not supported (missing internal source data set)";
        return;
    }

    auto & observationDS = *m_observationData->dataSet();
    auto & modelDS = *m_modelData->dataSet();

    auto transformedObservationCoords = dynamic_cast<CoordinateTransformableDataObject *>(m_observationData);
    auto transformedModelCoords = dynamic_cast<CoordinateTransformableDataObject *>(m_modelData);
    auto && coordinateSystem = currentCoordinateSystem();
    const bool useTransformedCoordinates =
        transformedObservationCoords && transformedModelCoords && coordinateSystem.isValid();

    auto observationDSTransformedPtr = useTransformedCoordinates
        ? transformedObservationCoords->coordinateTransformedDataSet(coordinateSystem)
        : vtkSmartPointer<vtkDataSet>(m_observationData->dataSet());
    auto modelDSTransformedPtr = useTransformedCoordinates
        ? transformedModelCoords->coordinateTransformedDataSet(coordinateSystem)
        : vtkSmartPointer<vtkDataSet>(m_modelData->dataSet());

    if (!observationDSTransformedPtr || !modelDSTransformedPtr)
    {
        qDebug() << "Invalid input coordinates or data sets";
        return;
    }

    auto & observationDSTransformed = *observationDSTransformedPtr;
    auto & modelDSTransformed = *modelDSTransformedPtr;

    const vtkSmartPointer<vtkDataArray> observationData = useObservationCellData
        ? observationDS.GetCellData()->GetArray(observationAttributeName.toUtf8().data())
        : observationDS.GetPointData()->GetArray(observationAttributeName.toUtf8().data());

    const vtkSmartPointer<vtkDataArray> modelData = useModelCellData
        ? modelDS.GetCellData()->GetArray(modelAttributeName.toUtf8().data())
        : modelDS.GetPointData()->GetArray(modelAttributeName.toUtf8().data());


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

        using LosDispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
        ProjectToLineOfSightWorker losWorker;
        losWorker.lineOfSight = m_inSARLineOfSight;
        if (!LosDispatcher::Execute(&displacement, losWorker))
        {
            losWorker(&displacement);
        }

        auto projected = losWorker.projectedData;

        auto projectedName = m_attributeNamesLocations[dataIndex].first + " (projected)";
        m_projectedAttributeNames[dataIndex] = projectedName;
        projected->SetName(projectedName.toUtf8().data());

        if (m_attributeNamesLocations[dataIndex].second)    // use cell data?
        {
            dataSet.GetCellData()->AddArray(projected);
        }
        else
        {
            dataSet.GetPointData()->AddArray(projected);
        }

        return projected;
    };


    auto observationLosDisp = getProjectedDisp(*observationData, observationIndex, observationDS);
    auto modelLosDisp = getProjectedDisp(*modelData, modelIndex, modelDS);
    assert(observationLosDisp && modelLosDisp);


    // Now interpolate one of the data arrays to the other's structure.
    // Use the data sets transformed to the user-selected coordinate system.
    if (m_interpolationMode == InterpolationMode::modelToObservation)
    {
        auto attributeName = QString::fromUtf8(modelLosDisp->GetName());
        modelLosDisp = InterpolationHelper::interpolate(
            observationDSTransformed, modelDSTransformed, attributeName, useModelCellData);
    }
    else
    {
        auto attributeName = QString::fromUtf8(observationLosDisp->GetName());
        observationLosDisp = InterpolationHelper::interpolate(
            modelDSTransformed, observationDSTransformed, attributeName, useObservationCellData);
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
        ? observationDSTransformed
        : modelDSTransformed;

    const double observationUnitFactor = std::pow(10, m_observationUnitDecimalExponent);
    const double modelUnitFactor = std::pow(10, m_modelUnitDecimalExponent);

    using ResidualDispatcher = vtkArrayDispatch::Dispatch2ByValueType<
        vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;
    ResidualWorker residualWorker;
    residualWorker.observationUnitFactor = observationUnitFactor;
    residualWorker.modelUnitFactor = modelUnitFactor;
    if (!ResidualDispatcher::Execute(observationLosDisp, modelLosDisp, residualWorker))
    {
        residualWorker(observationLosDisp.Get(), modelLosDisp.Get());
    }
    auto residualData = residualWorker.residual;
    residualData->SetName(m_attributeNamesLocations[residualIndex].first.toUtf8().data());

    assert(!m_newResidual);
    m_newResidual = vtkSmartPointer<vtkDataSet>::Take(referenceDataSet.NewInstance());
    m_newResidual->CopyStructure(&referenceDataSet);

    const bool resultInCellData = m_interpolationMode == InterpolationMode::modelToObservation
        ? m_attributeNamesLocations[observationIndex].second
        : m_attributeNamesLocations[modelIndex].second;
    const auto residualExpectedTuples = resultInCellData
        ? m_newResidual->GetNumberOfCells()
        : m_newResidual->GetNumberOfPoints();

    if (residualData->GetNumberOfTuples() != residualExpectedTuples)
    {
        qDebug() << "Residual creation failed: Unexpected output size.";
        return;
    }

    auto & resultAttributes = resultInCellData
        ? static_cast<vtkDataSetAttributes &>(*m_newResidual->GetCellData())
        : static_cast<vtkDataSetAttributes &>(*m_newResidual->GetPointData());
    resultAttributes.SetScalars(residualData);
}

void ResidualVerificationView::updateGuiAfterDataChange()
{
    if (!m_visToDeleteAfterUpdate.empty())
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
        {
            continue;
        }

        auto attributeName = m_projectedAttributeNames[i];
        if (attributeName.isEmpty())
        {
            attributeName = m_attributeNamesLocations[i].first;
        }

        m_visualizations[i]->colorMapping().setCurrentScalarsByName(attributeName);
        m_visualizations[i]->colorMapping().setEnabled(true);
        m_visualizations[i]->colorMapping().colorBarRepresentation().setVisible(true);
    }

    updateGuiSelection();

    QList<DataObject *> validInputData;
    if (m_observationData)
    {
        validInputData << m_observationData;
    }
    if (m_modelData)
    {
        validInputData << m_modelData;
    }
    m_implementation->strategy2D().setInputData(validInputData);

    if (!validInputData.isEmpty())
    {
        implementation().resetCamera(true, 0);
    }

    render();
}

void ResidualVerificationView::updateGuiSelection()
{
    updateTitle();

    if (implementation().selection().visualization)
    {
        return;
    }

    // It's more convenient to chose one of the contents as "current", even if none is directly selected.
    // Prefer to use the active sub view data.
    // Check for the visualization and the data object. If we are currently removing the data object,
    // the visualization might not be cleaned up yet.
    QVector<unsigned int> indices(numberOfViews);
    {
        unsigned int i = 0;
        std::generate(indices.begin(), indices.end(), [&i] () { return i++; });
        indices.removeOne(activeSubViewIndex());
        indices.prepend(activeSubViewIndex());
    }

    for (const auto i : indices)
    {
        const auto d = dataAt(i);
        const auto vis = m_visualizations[i].get();
        if (d && vis && (&vis->dataObject() == d))
        {
            setVisualizationSelection(VisualizationSelection(vis));
            return;
        }
    }
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
        {
            return false;
        }
        m_observationData = dataObject;
        break;
    case modelIndex:
        if (m_modelData == dataObject)
        {
            return false;
        }
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
            if (const auto name = scalars->GetName())
            {
                return std::make_pair(QString::fromUtf8(name), false);
            }
        }
        else if (auto firstArray = dataSet.GetPointData()->GetArray(0))
        {
            if (const auto name = firstArray->GetName())
            {
                return std::make_pair(QString::fromUtf8(name), false);
            }
        }

        if (auto scalars = dataSet.GetCellData()->GetScalars())
        {
            if (const auto name = scalars->GetName())
            {
                return std::make_pair(QString::fromUtf8(name), true);
            }
        }
        else if (auto firstArray = dataSet.GetCellData()->GetArray(0))
        {
            if (const auto name = firstArray->GetName())
            {
                return std::make_pair(QString::fromUtf8(name), true);
            }
        }
        return{};
    };

    auto checkDefaultVectors = [] (vtkDataSet & dataSet) -> std::pair<QString, bool>
    {
        if (auto scalars = dataSet.GetPointData()->GetVectors())
        {
            if (const auto name = scalars->GetName())
            {
                return std::make_pair(QString::fromUtf8(name), false);
            }
        }
        else
        {
            for (int i = 0; i < dataSet.GetPointData()->GetNumberOfArrays(); ++i)
            {
                auto array = dataSet.GetPointData()->GetArray(i);
                if (array->GetNumberOfComponents() != 3)
                {
                    continue;
                }

                if (const auto name = array->GetName())
                {
                    return std::make_pair(QString::fromUtf8(name), false);
                }
            }
        }

        if (auto scalars = dataSet.GetCellData()->GetVectors())
        {
            if (const auto name = scalars->GetName())
            {
                return std::make_pair(QString::fromUtf8(name), true);
            }
        }
        else
        {
            for (int i = 0; i < dataSet.GetCellData()->GetNumberOfArrays(); ++i)
            {
                auto array = dataSet.GetCellData()->GetArray(i);
                if (array->GetNumberOfComponents() != 3)
                {
                    continue;
                }

                if (const auto name = array->GetName())
                {
                    return std::make_pair(QString::fromUtf8(name), false);
                }
            }
        }
        return{};
    };

    if (inputType == observationIndex)
    {
        return checkDefaultScalars(dataSet);
    }

    if (inputType == modelIndex)
    {
        // Polygonal data is cell based, point and grid data is point based.
        bool cellBasedData = false;
        bool usePrimaryAttribute = true;

        if (auto poly = vtkPolyData::SafeDownCast(&dataSet))
        {
            if (auto polys = poly->GetPolys())
            {
                cellBasedData = polys->GetNumberOfCells() > 0;
            }
        }

        auto & primaryAttributes = cellBasedData
            ? static_cast<vtkDataSetAttributes &>(*dataSet.GetCellData())
            : static_cast<vtkDataSetAttributes &>(*dataSet.GetPointData());
        auto & secondaryAttributes = cellBasedData
            ? static_cast<vtkDataSetAttributes &>(*dataSet.GetPointData())
            : static_cast<vtkDataSetAttributes &>(*dataSet.GetCellData());

        static const auto modelArrayNames = {
            "Deformation", "deformation"
            "Displacement Vectors", "displacement vectors",
            "U-"
        };

        auto findDisplacementData = [] (vtkDataSetAttributes & attributes) -> vtkAbstractArray *
        {
            for (auto nameIt = modelArrayNames.begin(); nameIt != modelArrayNames.end(); ++nameIt)
            {
                if (auto array = attributes.GetAbstractArray(*nameIt))
                {
                    return array;
                }
            }
            if (auto vectors = attributes.GetVectors())
            {
                return vectors;
            }
            return attributes.GetScalars();
        };

        auto displacementData = findDisplacementData(primaryAttributes);
        if (!displacementData)
        {
            usePrimaryAttribute = false;
            displacementData = findDisplacementData(secondaryAttributes);
        }

        if (!displacementData)
        {
            return {};
        }

        const bool useCellData = usePrimaryAttribute ? cellBasedData : !cellBasedData;

        return std::make_pair(QString::fromUtf8(displacementData->GetName()), useCellData);
    }

    return std::make_pair(QString("Residual"), true);
}
