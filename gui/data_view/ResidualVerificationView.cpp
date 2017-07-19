/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gui/data_view/ResidualVerificationView.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>

#include <QBoxLayout>
#include <QProgressBar>
#include <QTimer>
#include <QToolBar>
#include <QtConcurrent/QtConcurrent>

#include <vtkCellData.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>

#include <core/AbstractVisualizedData.h>
#include <core/color_mapping/ColorBarRepresentation.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/DataSetHandler.h>
#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <core/utility/DataExtent.h>
#include <core/utility/DataSetResidualHelper.h>
#include <core/utility/macros.h>
#include <core/utility/vtkCameraSynchronization.h>
#include <core/utility/ScalarBarActor.h>
#include <core/utility/types_utils.h>
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
    , m_inResidualUpdate{ false }
    , m_deferringVisualizationUpdate{ false }
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

    m_implementation = std::make_unique<RendererImplementationResidual>(*this);
    m_implementation->activate(qvtkWidget());

    m_cameraSync = std::make_unique<vtkCameraSynchronization>();
    for (unsigned int i = 0; i < numberOfSubViews(); ++i)
    {
        m_cameraSync->add(m_implementation->renderer(i));
    }

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

void ResidualVerificationView::updateResidual()
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

    auto scalars = std::make_pair(QString(), IndexType::invalid);
    if (dataObject)
    {
        scalars = findDataSetAttributeName(*dataObject, subViewIndex);
        if (scalars.second == IndexType::invalid)
        {
            qDebug() << "Unsupported data object (no data found):" << dataObject->name();
        }
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

    emit inputDataChanged();
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

RendererImplementationResidual & ResidualVerificationView::implementationResidual() const
{
    assert(m_implementation);
    return *m_implementation;
}

void ResidualVerificationView::initializeRenderContext()
{
    assert(m_implementation);
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

void ResidualVerificationView::onCoordinateSystemChanged(const CoordinateSystemSpecification & spec)
{
    AbstractRenderView::onCoordinateSystemChanged(spec);

    m_residualHelper->setTargetCoordinateSystem(spec);
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
    assert(QThread::currentThread() == this->thread());

    // Correctly handle cases where this function is called recursively by the event loop.
    // Just wait until the previous update finished and update again, as requested.
    if (m_inResidualUpdate)
    {
        // Wait until the update thread finished and rerun the event loop to handle the residual
        // results. Only then continue with the new computation.
        waitForResidualUpdate();
    }

    if (m_destructorCalled)
    {
        return;
    }

    m_inResidualUpdate = true;

    m_progressBar->show();
    toolBar()->setEnabled(false);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    assert(!m_residualHelper->residualDataObject());

    // While updating the residual, we might add transformed vector data to the input data sets.
    // Make sure that this modification on the DataObject internals does not conflict with GUI events.
    // Lock all currently available data objects.
    m_eventDeferrals.resize(3);
    m_eventDeferrals[observationIndex] = ScopedEventDeferral{ m_residualHelper->observationDataObject() };
    m_eventDeferrals[modelIndex] = ScopedEventDeferral{ m_residualHelper->modelDataObject() };
    m_eventDeferrals[residualIndex] = ScopedEventDeferral{ m_residual.get() };

    auto future = QtConcurrent::run(this, &ResidualVerificationView::updateResidualInternal);
    m_updateWatcher->setFuture(future);
}

void ResidualVerificationView::handleUpdateFinished()
{
    if (auto newResidual = m_residualHelper->takeResidualDataObject())
    {
        vtkSmartPointer<vtkDataSet> computedDataSet = newResidual->dataSet();

        DEBUG_ONLY(const bool residualIsImage = nullptr != vtkImageData::SafeDownCast(computedDataSet);)
        assert(residualIsImage || vtkPolyData::SafeDownCast(computedDataSet));

        const bool reuseResidualObject = m_residual
            && (m_residual->dataTypeName() == newResidual->dataTypeName());

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

    m_eventDeferrals.clear();

    updateGuiAfterDataChange();

    toolBar()->setEnabled(true);
    m_progressBar->hide();

    m_inResidualUpdate = false;
}

void ResidualVerificationView::updateResidualInternal()
{
    assert(!m_residualHelper->residualDataObject());

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
    cleanOldGuiData();
    updateVisualizations();
}

void ResidualVerificationView::cleanOldGuiData()
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

}

void ResidualVerificationView::updateVisualizations()
{
    auto timer = dynamic_cast<QTimer *>(sender());

    // Check if there already is an update on the list. Don't refresh the visualizations twice.
    if (!timer && m_deferringVisualizationUpdate)
    {
        return;
    }

    if (timer)
    {
        timer->deleteLater();
    }

    // Wait until locks on the current data are released.
    const bool isLocked = std::any_of(m_visualizations.begin(), m_visualizations.end(),
        [] (const std::unique_ptr<AbstractVisualizedData> & vis)
    {
        return vis && vis->dataObject().isDeferringEvents();
    });
    if (isLocked)
    {
        m_deferringVisualizationUpdate = true;
        auto deferredUpdate = new QTimer(this);
        connect(deferredUpdate, &QTimer::timeout, this, &ResidualVerificationView::updateVisualizations);
        deferredUpdate->start();
        return;
    }

    m_deferringVisualizationUpdate = false;

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

        auto & colorMapping = m_visualizations[i]->colorMapping();
        colorMapping.colorBarRepresentation().setPosition(ColorBarRepresentation::posBottom);
        colorMapping.setCurrentScalarsByName(attributeName, true);
        colorMapping.colorBarRepresentation().setVisible(true);

        if (i == 2)
        {
            colorMapping.setManualGradient(residualGradient());
            colorMapping.colorBarRepresentation().actor().Modified();
        }
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
    DataObject & dataObject, unsigned int inputType) const
{
    // Residual: location derived from source geometry when the residual is computed
    if (inputType == residualIndex)
    {
        return std::make_pair(m_residualHelper->residualDataObjectName(), IndexType::invalid);
    }

    // If displacement vector data is used, it will be projected to line of sight displacements.
    // The projected data needs to be stored in the underlaying data set to be usable in color mappings.
    if (!dataObject.dataSet())
    {
        return std::make_pair(QString(), IndexType::invalid);
    }

    auto dataSet = dataObject.processedOutputDataSet();
    if (!dataSet)
    {
        return std::make_pair(QString(), IndexType::invalid);
    }

    // Search default deformation attribute in the scalar association defined by the data object.
    const auto primaryLocation = IndexType_util(dataObject.defaultAttributeLocation());
    auto secondaryLocation = IndexType_util(IndexType::invalid);

    // Polygonal data may also have useful attributes associated with points.
    // For other data sets (images, point clouds), no secondary location is considered.
    if (vtkPolyData::SafeDownCast(dataSet))
    {
        if (primaryLocation == IndexType::cells)
        {
            secondaryLocation = IndexType_util(IndexType::points);
        }
    }

    auto primaryAttributes = primaryLocation.extractAttributes(dataSet);
    auto secondaryAttributes = secondaryLocation.extractAttributes(dataSet);

    // Search deformation attribute by name first, than check if scalars or vectors are available.
    // Always look in the primary attribute location first.

    static const auto deformationAttributeNames = {
        "Deformation", "deformation",
        "Deformation Vectors", "deformation vectors",
        "Deformation Vector", "deformation vector",
        "Displacement Vectors", "displacement vectors",
        "Displacement Vector", "displacement vector",
        "U-", "u-"
    };

    auto searchValidAttribute = [] (vtkDataSetAttributes * attributes) -> QString
    {
        if (!attributes)
        {
            return{};
        }

        for (auto && name : deformationAttributeNames)
        {
            if (attributes->HasArray(name))
            {
                return name;
            }
        }
        if (auto scalars = attributes->GetScalars())
        {
            auto name = scalars->GetName();
            if (name && name[0] != '\0')
            {
                return QString::fromUtf8(name);
            }
        }
        if (auto vectors = attributes->GetVectors())
        {
            auto name = vectors->GetName();
            if (name && name[0] != '\0')
            {
                return QString::fromUtf8(name);
            }
        }
        return{};
    };

    auto name = searchValidAttribute(primaryAttributes);
    if (!name.isEmpty())
    {
        return std::make_pair(name, primaryLocation);
    }
    name = searchValidAttribute(secondaryAttributes);
    if (!name.isEmpty())
    {
        return std::make_pair(name, secondaryLocation);
    }

    return std::make_pair(QString(), IndexType::invalid);
}

vtkLookupTable & ResidualVerificationView::residualGradient()
{
    if (!m_residualGradient)
    {
        m_residualGradient = vtkSmartPointer<vtkLookupTable>::New();
    }

    if (m_residual && m_residual->dataSet())
    {
        if (auto res = m_residual->dataSet()->GetPointData()->GetScalars())
        {
            ValueRange<double> valueRange;
            res->GetRange(valueRange.data());

            // Blue for negative parts, red for positives
            const double whitePos = std::max(0.0, std::min(1.0, valueRange.relativeOriginPosition()));
            const int numColors = 255;
            m_residualGradient->SetNumberOfTableValues(numColors);

            const int numBlue = static_cast<int>(std::round(numColors * whitePos));
            const int numRed = numColors - numBlue;
            if (numBlue == 1)
            {
                m_residualGradient->SetTableValue(0, 0.0, 0.0, 1.0);
            }
            else for (int i = 0; i < numBlue; ++i)
            {
                const double s = static_cast<double>(i) / (numBlue - 1);
                m_residualGradient->SetTableValue(i, s, s, 1.0);
            }

            if (numRed == 1)
            {
                m_residualGradient->SetTableValue(numColors - 1, 1.0, 0.0, 0.0);
            }
            else for (int i = 0; i < numRed; ++i)
            {
                const double s = 1.0 - static_cast<double>(i) / (numRed - 1);
                m_residualGradient->SetTableValue(i + numBlue, 1.0, s, s);
            }

            m_residualGradient->SetNanColor(1, 1, 1, 1);
            m_residualGradient->BuildSpecialColors();
        }
    }

    return *m_residualGradient;
}
