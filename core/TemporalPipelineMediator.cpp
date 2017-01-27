#include "TemporalPipelineMediator.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

#include <QDebug>

#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkExecutive.h>
#include <vtkInformation.h>
#include <vtkPointData.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <core/data_objects/DataObject.h>
#include <core/AbstractVisualizedData.h>


TemporalPipelineMediator::TemporalPipelineMediator()
    : QObject()
    , m_visualization{}
    , m_pipelineModifiedTime{}
    , m_selection{}
{
}

TemporalPipelineMediator::~TemporalPipelineMediator() = default;

void TemporalPipelineMediator::setVisualization(AbstractVisualizedData * visualization)
{
    if (visualization == m_visualization)
    {
        return;
    }

    m_visualization = visualization;

    m_timeSteps.clear();
    m_pipelineModifiedTime = {};
    m_selection = {};

    updateTimeSteps();

    assert((m_visualization != nullptr) || m_timeSteps.empty());

    if (m_timeSteps.empty())
    {
        return;
    }

    // Check if there is a valid time step set in the pipeline
    if (selectionFromPipeline())
    {
        return;
    }

    // Or just default to the first one
    m_selection.selectedTimeStep = m_timeSteps.front();
    passSelectionToPipeline();

    // One time initialization: the temporal array was not visible before, so other components
    // need to be informed of the modification
    if (auto dataSet = visualization->dataObject().dataSet())
    {
        const ScopedEventDeferral deferral(visualization->dataObject());
        dataSet->GetPointData()->Modified();
        dataSet->GetCellData()->Modified();
    }
}

AbstractVisualizedData * TemporalPipelineMediator::visualzation()
{
    return m_visualization;
}

const AbstractVisualizedData * TemporalPipelineMediator::visualzation() const
{
    return m_visualization;
}

const std::vector<double> & TemporalPipelineMediator::timeSteps() const
{
    return m_timeSteps;
}

void TemporalPipelineMediator::selectTimeStepByIndex(size_t index)
{
    // TODO push notification instead of polling?
    const bool timeStepsChanged = updateTimeSteps();

    if (index >= m_timeSteps.size())
    {
        qWarning() << "Invalid time step index:" << index << ", have only" << m_timeSteps.size();
        return;
    }

    if (!timeStepsChanged
        && m_selection.index == index
        && m_selection.selectedTimeStep == m_timeSteps[index])
    {
        return;
    }

    m_selection.index = index;
    m_selection.selectedTimeStep = m_timeSteps[index];

    if (!m_visualization)
    {
        return;
    }

    passSelectionToPipeline();
}

size_t TemporalPipelineMediator::currentTimeStepIndex() const
{
    return m_selection.index;
}

double TemporalPipelineMediator::selectedTimeStep() const
{
    return m_selection.selectedTimeStep;
}

double TemporalPipelineMediator::nullTimeStep()
{
    return std::numeric_limits<double>::quiet_NaN();
}

bool TemporalPipelineMediator::isValidTimeStep(double timeStep)
{
    return !std::isnan(timeStep);
}

double TemporalPipelineMediator::currentUpdateTimeStep(AbstractVisualizedData & visualization,
    const unsigned int index)
{
    if (visualization.numberOfOutputPorts() < index)
    {
        return nullTimeStep();
    }

    auto producer = visualization.processedOutputPort(index)->GetProducer();
    if (!producer)
    {
        return nullTimeStep();
    }

    producer->UpdateInformation();
    auto outInfo = producer->GetOutputInformation(0);
    if (!outInfo || !outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
        return nullTimeStep();
    }

    return outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
}

bool TemporalPipelineMediator::updateTimeSteps()
{
    if (!m_visualization)
    {
        const bool modified = !m_timeSteps.empty();
        (decltype(m_timeSteps)()).swap(m_timeSteps);
        return modified;
    }

    vtkMTimeType currentUpdateTime = {};

    // Check if there is new pipeline information
    for (unsigned int port = 0; port < m_visualization->numberOfOutputPorts(); ++port)
    {
        auto outputPort = m_visualization->processedOutputPort(port);
        if (!outputPort || !outputPort->GetProducer())
        {
            continue;
        }

        auto algorithm = outputPort->GetProducer();

        if (algorithm->GetExecutive()->UpdateInformation() == 0)
        {
            qWarning() << "Algorithm execution unsuccessful for data set" << m_visualization->dataObject().name();
            continue;
        }

        currentUpdateTime = std::max(currentUpdateTime, algorithm->GetOutputInformation(port)->GetMTime());
    }

    if (currentUpdateTime == m_pipelineModifiedTime)
    {
        return false;
    }

    m_pipelineModifiedTime = currentUpdateTime;

    std::vector<double> timeSteps;

    // Retrieve complete list of time steps for all visualization output ports
    for (unsigned int port = 0; port < m_visualization->numberOfOutputPorts(); ++port)
    {
        auto outputPort = m_visualization->processedOutputPort(port);
        if (!outputPort || !outputPort->GetProducer() || !outputPort->GetProducer()->GetOutputInformation(port))
        {
            continue;
        }

        auto algorithm = outputPort->GetProducer();

        auto & outInfo = *algorithm->GetOutputInformation(port);
        if (!outInfo.Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
        {
            continue;
        }

        assert(outInfo.Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) >= 0);
        const auto numTimeStepsForPort = static_cast<size_t>(
            outInfo.Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()));
        timeSteps.resize(timeSteps.size() + numTimeStepsForPort);
        const auto numPreviousTimeSteps = timeSteps.size() - numTimeStepsForPort;
        outInfo.Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeSteps.data() + numPreviousTimeSteps);

        assert(std::is_sorted(timeSteps.begin() + numPreviousTimeSteps, timeSteps.end()));
        // Remove all new time steps that are already in the list.
        const auto newEndIt = std::unique(timeSteps.begin(), timeSteps.end());
        timeSteps.erase(newEndIt, timeSteps.end());
        // Restore sorting of remaining time steps
        std::inplace_merge(timeSteps.begin(),
            timeSteps.begin() + numPreviousTimeSteps,
            timeSteps.end());
    }

    if (m_timeSteps == timeSteps)
    {
        return false;
    }

    std::swap(m_timeSteps, timeSteps);
    return true;
}

bool TemporalPipelineMediator::selectionFromPipeline()
{
    if (!m_visualization)
    {
        return false;
    }

    bool hasPreviousTimeStep = false;
    double previousTimeStep = {};
    size_t previousTimeStepIndex = {};

    // Search for a previous update time step
    for (unsigned int port = 0; port < m_visualization->numberOfOutputPorts(); ++port)
    {
        const auto outputPort = m_visualization->processedOutputPort(port);
        auto & algoritm = *outputPort->GetProducer();
        auto & outInfo = *algoritm.GetOutputInformation(outputPort->GetIndex());
        if (!outInfo.Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
        {
            continue;
        }
        const auto timeStep = outInfo.Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
        const auto it = std::lower_bound(m_timeSteps.begin(), m_timeSteps.end(), timeStep);
        if (it == m_timeSteps.end() || *it != timeStep)
        {
            continue;
        }

        hasPreviousTimeStep = true;
        previousTimeStep = timeStep;
        previousTimeStepIndex = static_cast<size_t>(it - m_timeSteps.begin());
        break;
    }

    if (!hasPreviousTimeStep)
    {
        return false;
    }

    m_selection.index = previousTimeStepIndex;
    m_selection.selectedTimeStep = previousTimeStep;
    // Make sure that all ports refer to the same time step
    passSelectionToPipeline();

    return true;
}

void TemporalPipelineMediator::passSelectionToPipeline()
{
    if (!m_visualization)
    {
        return;
    }

    for (unsigned int port = 0; port < m_visualization->numberOfOutputPorts(); ++port)
    {
        const auto outputPort = m_visualization->processedOutputPort(port);
        auto & algoritm = *outputPort->GetProducer();
        auto & outInfo = *algoritm.GetOutputInformation(outputPort->GetIndex());
        outInfo.Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), m_selection.selectedTimeStep);

        // Enforce updating downstream algorithms
        // TODO Is this the best way to go here?
        algoritm.Modified();
    }

    emit m_visualization->geometryChanged();
}

TemporalPipelineMediator::TimeStepSelection::TimeStepSelection()
    : index{ 0 }
    , selectedTimeStep{ 0.0 }
{
}
