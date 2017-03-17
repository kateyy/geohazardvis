#include <gui/data_view/ResidualVerificationView.h>

#include <cassert>

#include <QBoxLayout>
#include <QProgressBar>
#include <QToolBar>
#include <QtConcurrent/QtConcurrent>

#include <vtkCellData.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>

#include <core/AbstractVisualizedData.h>
#include <core/color_mapping/ColorBarRepresentation.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/DataSetHandler.h>
#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <core/utility/DataSetResidualHelper.h>
#include <core/utility/macros.h>
#include <core/utility/vtkCameraSynchronization.h>
#include <gui/DataMapping.h>
#include <gui/data_view/RendererImplementationResidual.h>
#include <gui/data_view/RenderViewStrategy2D.h>


ResidualVerificationView::ResidualVerificationView(DataMapping & dataMapping, int index, QWidget * parent, Qt::WindowFlags flags)
    : AbstractRenderView(dataMapping, index, parent, flags)
    , m_residualHelper{ std::make_unique<DataSetResidualHelper>() }
    , m_residualGeometrySource{ InputData::model }
    , m_observationUnitDecimalExponent{ 0 }
    , m_modelUnitDecimalExponent{ 0 }
    , m_updateWatcher{ std::make_unique<QFutureWatcher<void>>() }
    , m_destructorCalled{ false }
{
    m_residualHelper->setGeometrySource(m_residualGeometrySource == InputData::observation
        ? DataSetResidualHelper::InputData::Observation
        : DataSetResidualHelper::InputData::Model);

    connect(m_updateWatcher.get(), &QFutureWatcher<void>::finished,
        this, &ResidualVerificationView::handleUpdateFinished);

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
    m_visualizations = {};

    if (m_residual)
    {
        dataSetHandler().removeExternalData({ m_residual.get() });
        dataMapping().removeDataObjects({ m_residual.get() });
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
    return !m_residualHelper->observationDataObject() && !m_residualHelper->modelDataObject();
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
    return m_residualHelper->observationDataObject();
}

DataObject * ResidualVerificationView::modelData()
{
    return m_residualHelper->modelDataObject();
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

    m_residualHelper->setObservationScalarsScale(std::pow(10, exponent));

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

    m_residualHelper->setModelScalarsScale(std::pow(10, exponent));

    emit unitDecimalExponentsChanged(m_observationUnitDecimalExponent, m_modelUnitDecimalExponent);
}

void ResidualVerificationView::setDeformationLineOfSight(const vtkVector3d & los)
{
    m_residualHelper->setDeformationLineOfSight(los);

    emit lineOfSightChanged(los);
}

const vtkVector3d & ResidualVerificationView::deformationLineOfSight() const
{
    return m_residualHelper->deformationLineOfSight();
}

void ResidualVerificationView::setResidualGeometrySource(InputData geometrySource)
{
    if (m_residualGeometrySource == geometrySource)
    {
        return;
    }

    m_residualGeometrySource = geometrySource;

    m_residualHelper->setGeometrySource(geometrySource == InputData::observation
        ? DataSetResidualHelper::InputData::Observation
        : DataSetResidualHelper::InputData::Model);

    updateResidualAsync();

    emit residualGeometrySourceChanged(geometrySource);
}

ResidualVerificationView::InputData ResidualVerificationView::residualGeometrySource() const
{
    return m_residualGeometrySource;
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

void ResidualVerificationView::setDataHelper(
    unsigned int subViewIndex,
    DataObject * dataObject,
    bool skipResidualUpdate)
{
    assert(subViewIndex != residualIndex);

    if (dataAt(subViewIndex) == dataObject)
    {
        return;
    }

    setInputDataInternal(subViewIndex, dataObject);

    vtkSmartPointer<vtkDataSet> dataSet = dataObject ? dataObject->dataSet() : nullptr;
    if (dataObject && !dataSet)
    {
        qDebug() << "Unsupported data object (no data found):" << dataObject->name();
    }
    auto scalars = std::make_pair(QString(), IndexType::invalid);
    if (dataObject && dataSet)
    {
        scalars = findDataSetAttributeName(*dataSet, subViewIndex);
    }

    if (subViewIndex == observationIndex)
    {
        m_residualHelper->setObservationScalars(scalars.first, scalars.second);
    }
    else if (subViewIndex == modelIndex)
    {
        m_residualHelper->setModelScalars(scalars.first, scalars.second);
    }
    else
    {
        assert(false);
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
    // Ignore requests related to the residual
    enum { numberOfViewsToCheck = numberOfViews - 1 };

    // check if this request is relevant
    bool relevant = false;
    std::array<bool, numberOfViewsToCheck> hideSubViewObject = { false };
    if (subViewIndex >= 0 && subViewIndex < numberOfViewsToCheck)
    {
        relevant = hideSubViewObject[subViewIndex] = dataObjects.contains(dataAt(subViewIndex));
    }
    else
    {
        for (unsigned int i = 0; i < numberOfViewsToCheck; ++i)
        {
            hideSubViewObject[i] = dataObjects.contains(dataAt(i));
            relevant = relevant || hideSubViewObject[i];
        }
    }

    if (!relevant)
    {
        return;
    }

    waitForResidualUpdate();

    // no caching for now, just remove the objects
    for (unsigned int i = 0; i < numberOfViewsToCheck; ++i)
    {
        if (hideSubViewObject[i])
        {
            setDataHelper(i, nullptr, true);
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
        for (unsigned int i = 0; i < numberOfSubViews(); ++i)
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
    const bool unsetObservation = dataObjects.contains(m_residualHelper->observationDataObject());
    const bool unsetModel = dataObjects.contains(m_residualHelper->modelDataObject());

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

void ResidualVerificationView::onCoordinateSystemChanged(const CoordinateSystemSpecification & spec)
{
    AbstractRenderView::onCoordinateSystemChanged(spec);

    m_residualHelper->setTargetCoordinateSystem(spec);
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

void ResidualVerificationView::setInputDataInternal(unsigned int subViewIndex, DataObject * newData)
{
    assert(m_implementation);
    assert(subViewIndex != residualIndex);

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

    setDataAt(subViewIndex, newData);

    updateVisualizationForSubView(subViewIndex, newData);
}

void ResidualVerificationView::setResidualDataInternal(std::unique_ptr<DataObject> newResidual)
{
    assert(m_implementation);

    m_oldResidualToDeleteAfterUpdate = std::move(m_residual);
    if (m_oldResidualToDeleteAfterUpdate)
    {
        dataMapping().removeDataObjects({ m_oldResidualToDeleteAfterUpdate.get() });
        dataSetHandler().removeExternalData({ m_oldResidualToDeleteAfterUpdate.get() });
    }

    m_residual = std::move(newResidual);

    if (m_residual)
    {
        auto transformableResidual = dynamic_cast<CoordinateTransformableDataObject *>(m_residual.get());
        auto transformableObservation = dynamic_cast<CoordinateTransformableDataObject *>(m_residualHelper->observationDataObject());
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

        // Update UI/rendering for the newly created residual object
        dataSetHandler().addExternalData({ m_residual.get() });
    }

    updateVisualizationForSubView(residualIndex, m_residual.get());
}

void ResidualVerificationView::updateVisualizationForSubView(unsigned int subViewIndex, DataObject * newData)
{
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

    std::unique_lock<std::recursive_mutex> updateLock(m_updateMutex);

    if (m_destructorCalled)
    {
        return;
    }

    m_progressBar->show();
    toolBar()->setEnabled(false);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    assert(!m_residualHelper->residualDataObject());

    // While updating the residual, we might add transformed vector data to the input data sets.
    // Make sure that this modification on the DataObject internals does not conflict with GUI events.
    // Lock all currently available data objects.
    m_eventDeferrals[observationIndex] = ScopedEventDeferral{ m_residualHelper->observationDataObject() };
    m_eventDeferrals[modelIndex] = ScopedEventDeferral{ m_residualHelper->modelDataObject() };
    m_eventDeferrals[residualIndex] = ScopedEventDeferral{ m_residual.get() };

    m_updateMutexLock = std::move(updateLock);
    auto future = QtConcurrent::run(this, &ResidualVerificationView::updateResidualInternal);
    m_updateWatcher->setFuture(future);
}

void ResidualVerificationView::handleUpdateFinished()
{
    const auto lock = std::move(m_updateMutexLock);

    if (auto newResidual = m_residualHelper->takeResidualDataObject())
    {
        vtkSmartPointer<vtkDataSet> computedDataSet = newResidual->dataSet();

        DEBUG_ONLY(const bool residualIsImage = nullptr != vtkImageData::SafeDownCast(computedDataSet);)
        assert(residualIsImage || vtkPolyData::SafeDownCast(computedDataSet));

        const bool reuseResidualObject = m_residual && m_residual->dataSet()->IsA(computedDataSet->GetClassName());

        if (reuseResidualObject)
        {
            const ScopedEventDeferral residualDeferal(*m_residual);
            m_residual->dataSet()->CopyStructure(computedDataSet);
            m_residual->dataSet()->CopyAttributes(computedDataSet);
        }
        else
        {
            setResidualDataInternal(std::move(newResidual));
        }
    }
    else if (m_residual)
    {
        // just remove the old residual

        setResidualDataInternal(nullptr);
    }

    m_eventDeferrals = {};

    updateGuiAfterDataChange();

    toolBar()->setEnabled(true);
    m_progressBar->hide();
}

void ResidualVerificationView::updateResidualInternal()
{
    assert(!m_residualHelper->residualDataObject());
    assert(!m_updateMutex.try_lock());

    if (!m_residualHelper->updateResidual())
    {
        if (m_residualHelper->isSetupComplete())
        {
            // If the user provided all necessary data, the residual computation should not fail.
            qDebug() << "Residual computation failed";
        }
    }
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

        const auto attributeName = i == 0
            ? m_residualHelper->losObservationScalarsName()
            : (i == 1
                ? m_residualHelper->losModelScalarsName()
                : m_residualHelper->residualDataObjectName());

        m_visualizations[i]->colorMapping().setCurrentScalarsByName(attributeName);
        m_visualizations[i]->colorMapping().setEnabled(true);
        m_visualizations[i]->colorMapping().colorBarRepresentation().setVisible(true);
    }

    updateGuiSelection();

    QList<DataObject *> validInputData;
    if (auto observation = m_residualHelper->observationDataObject())
    {
        validInputData << observation;
    }
    if (auto model = m_residualHelper->modelDataObject())
    {
        validInputData << model;
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
        return m_residualHelper->observationDataObject();
    case modelIndex:
        return m_residualHelper->modelDataObject();
    case residualIndex:
        return m_residual.get();
    }
    assert(false);
    return nullptr;
}

void ResidualVerificationView::setDataAt(unsigned int i, DataObject * dataObject)
{
    switch (i)
    {
    case observationIndex:
        m_residualHelper->setObservationDataObject(dataObject);
        break;
    case modelIndex:
        m_residualHelper->setModelDataObject(dataObject);
        break;
    default:
        assert(false);
    }
}

std::pair<QString, IndexType> ResidualVerificationView::findDataSetAttributeName(
    vtkDataSet & dataSet, unsigned int inputType)
{
    auto checkDefaultScalars = [] (vtkDataSet & dataSet) -> std::pair<QString, IndexType>
    {
        if (auto scalars = dataSet.GetPointData()->GetScalars())
        {
            if (const auto name = scalars->GetName())
            {
                return std::make_pair(QString::fromUtf8(name), IndexType::points);
            }
        }
        else if (auto firstArray = dataSet.GetPointData()->GetArray(0))
        {
            if (const auto name = firstArray->GetName())
            {
                return std::make_pair(QString::fromUtf8(name), IndexType::points);
            }
        }

        if (auto scalars = dataSet.GetCellData()->GetScalars())
        {
            if (const auto name = scalars->GetName())
            {
                return std::make_pair(QString::fromUtf8(name), IndexType::cells);
            }
        }
        else if (auto firstArray = dataSet.GetCellData()->GetArray(0))
        {
            if (const auto name = firstArray->GetName())
            {
                return std::make_pair(QString::fromUtf8(name), IndexType::cells);
            }
        }
        return{};
    };

    auto checkDefaultVectors = [] (vtkDataSet & dataSet) -> std::pair<QString, IndexType>
    {
        if (auto scalars = dataSet.GetPointData()->GetVectors())
        {
            if (const auto name = scalars->GetName())
            {
                return std::make_pair(QString::fromUtf8(name), IndexType::points);
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
                    return std::make_pair(QString::fromUtf8(name), IndexType::points);
                }
            }
        }

        if (auto scalars = dataSet.GetCellData()->GetVectors())
        {
            if (const auto name = scalars->GetName())
            {
                return std::make_pair(QString::fromUtf8(name), IndexType::cells);
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
                    return std::make_pair(QString::fromUtf8(name), IndexType::points);
                }
            }
        }
        return std::make_pair(QString(), IndexType::invalid);
    };

    if (inputType == observationIndex)
    {
        return checkDefaultScalars(dataSet);
    }

    if (inputType == modelIndex)
    {
        // Polygonal data is cell based, point and grid data is point based.
        auto primaryLocation = IndexType::points;
        bool usePrimaryAttribute = true;

        if (auto poly = vtkPolyData::SafeDownCast(&dataSet))
        {
            if (auto polys = poly->GetPolys())
            {
                primaryLocation = polys->GetNumberOfCells() > 0 ? IndexType::cells : IndexType::points;
            }
        }

        const auto secondaryLocation = primaryLocation == IndexType::points
            ? IndexType::cells : IndexType::points;

        auto & primaryAttributes = primaryLocation == IndexType::cells
            ? static_cast<vtkDataSetAttributes &>(*dataSet.GetCellData())
            : static_cast<vtkDataSetAttributes &>(*dataSet.GetPointData());
        auto & secondaryAttributes = secondaryLocation == IndexType::cells
            ? static_cast<vtkDataSetAttributes &>(*dataSet.GetCellData())
            : static_cast<vtkDataSetAttributes &>(*dataSet.GetPointData());

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
            return std::make_pair(QString(), IndexType::invalid);
        }

        const auto location = usePrimaryAttribute ? primaryLocation : secondaryLocation;
        return std::make_pair(QString::fromUtf8(displacementData->GetName()), location);
    }

    // Residual: location derived from source geometry when the residual is computed
    return std::make_pair(QString("Residual"), IndexType::invalid);
}
