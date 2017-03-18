#include "TemporalPipelineMediator.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <utility>

#include <QDebug>

#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkExecutive.h>
#include <vtkInformation.h>
#include <vtkPointData.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <core/AbstractVisualizedData.h>
#include <core/data_objects/DataObject.h>
#include <core/filters/ExtractTimeStep.h>
#include <core/utility/macros.h>


namespace
{

const AbstractVisualizedData::StaticProcessingStepCookie & extractTimeStepPPCookie()
{
    static const auto cookie = AbstractVisualizedData::requestProcessingStepCookie();
    return cookie;
}

}


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

    // Or just select a default time step
    m_selection.index = m_timeSteps.size() / 2;
    m_selection.selectedTimeStep = m_timeSteps[m_selection.index];
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

void TemporalPipelineMediator::unregisterFromVisualization(AbstractVisualizedData * visualization)
{
    if (!visualization)
    {
        return;
    }

    if (visualization == this->visualzation())
    {
        setVisualization(nullptr);
    }

    bool modified = false;

    const auto numPorts = visualization->numberOfOutputPorts();
    for (unsigned int port = 0; port < numPorts; ++port)
    {
        bool portModified = visualization->erasePostProcessingStep(extractTimeStepPPCookie(), port);
        modified = modified || portModified;
    }

    if (modified)
    {
        visualization->geometryChanged();
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
    const unsigned int port)
{
    auto stepPtr = visualization.getPostProcessingStep(extractTimeStepPPCookie(), port);
    if (!stepPtr)
    {
        return nullTimeStep();
    }

    auto extractTimeStep = ExtractTimeStep::SafeDownCast(stepPtr->pipelineHead);
    assert(extractTimeStep);

    return extractTimeStep->GetTimeStep();
}

bool TemporalPipelineMediator::updateTimeSteps()
{
    if (!m_visualization)
    {
        const bool modified = !m_timeSteps.empty();
        m_timeSteps.clear();
        return modified;
    }

    vtkSmartPointer<vtkAlgorithm> upstreamAlgorithm;
    int upstreamOutputPort = 0;

    // If a ExtractTimeStep was already injected, processedOutputPort() does not report the
    // list of time steps anymore.
    const auto numPorts = m_visualization->numberOfOutputPorts();
    for (unsigned int port = 0; port < numPorts; ++port)
    {
        auto ppStep = m_visualization->getPostProcessingStep(extractTimeStepPPCookie(), port);
        if (!ppStep)
        {
            continue;
        }

        assert(ppStep->pipelineHead);
        upstreamAlgorithm = ppStep->pipelineHead->GetInputAlgorithm();
        assert(upstreamAlgorithm);
    }

    if (!upstreamAlgorithm)
    {
        // Assume there always is a valid port 0
        auto outputPort = m_visualization->processedOutputPort();
        upstreamAlgorithm = outputPort->GetProducer();
        assert(upstreamAlgorithm);
        upstreamOutputPort = outputPort->GetIndex();
    }

    if (!upstreamAlgorithm)
    {
        qWarning() << "Invalid output port 0 in data set" << m_visualization->dataObject().name();
        return false;
    }

    // Check if there is new pipeline information
    // Assume that all visualization ports report the same time steps
    if (upstreamAlgorithm->GetExecutive()->UpdateInformation() == 0)
    {
        qWarning() << "Algorithm execution unsuccessful for data set" << m_visualization->dataObject().name();
        return false;
    }

    const auto currentUpdateTime = upstreamAlgorithm->GetOutputInformation(upstreamOutputPort)->GetMTime();

    if (currentUpdateTime == m_pipelineModifiedTime)
    {
        return false;
    }

    m_pipelineModifiedTime = currentUpdateTime;

    std::vector<double> timeSteps;

    do
    {
        auto & outInfo = *upstreamAlgorithm->GetOutputInformation(upstreamOutputPort);
        if (!outInfo.Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
        {
            break;
        }

        assert(outInfo.Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) >= 0);
        const auto numTimeSteps = static_cast<size_t>(
            outInfo.Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()));
        const auto ptr = outInfo.Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

        timeSteps = std::vector<double>(ptr, ptr + numTimeSteps);

    } while (false);

    if (m_timeSteps == timeSteps)
    {
        return false;
    }

    std::swap(m_timeSteps, timeSteps);
    return true;
}

bool TemporalPipelineMediator::selectionFromPipeline()
{
    // Nothing to do if there is no visualization or it has not time steps.
    if (!m_visualization || m_timeSteps.empty())
    {
        return false;
    }

    bool hasPreviousTimeStep = false;
    double previousTimeStep = {};
    size_t previousTimeStepIndex = {};

    const auto numPorts = m_visualization->numberOfOutputPorts();

    // Check for previously injected processing steps
    for (unsigned int port = 0; port < numPorts; ++port)
    {
        auto stepPtr = m_visualization->getPostProcessingStep(extractTimeStepPPCookie(), port);
        if (!stepPtr)
        {
            continue;
        }
        auto & step = *stepPtr;

        assert(step.pipelineHead == step.pipelineTail);
        auto algorithm = ExtractTimeStep::SafeDownCast(step.pipelineHead);
        assert(algorithm);

        hasPreviousTimeStep = true;

        auto it = std::lower_bound(m_timeSteps.begin(), m_timeSteps.end(), algorithm->GetTimeStep());
        if (it == m_timeSteps.end())
        {
            previousTimeStepIndex = m_timeSteps.size() - 1u;
        }
        else
        {
            previousTimeStepIndex = static_cast<size_t>(it - m_timeSteps.begin());
        }
        // in case there was no exact match
        previousTimeStep = m_timeSteps[previousTimeStepIndex];
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

    bool modified = false;

    const auto numPorts = m_visualization->numberOfOutputPorts();
    for (unsigned int port = 0; port < numPorts; ++port)
    {
        vtkSmartPointer<ExtractTimeStep> extractTimeStep;

        auto ppStepPtr = m_visualization->getPostProcessingStep(extractTimeStepPPCookie(), port);
        const bool needToInject = ppStepPtr == nullptr;
        if (ppStepPtr)
        {
            extractTimeStep = ExtractTimeStep::SafeDownCast(ppStepPtr->pipelineHead);
            assert(extractTimeStep && (ppStepPtr->pipelineHead == ppStepPtr->pipelineTail));
        }
        else
        {
            extractTimeStep = vtkSmartPointer<ExtractTimeStep>::New();
        }

        modified = modified || (extractTimeStep->GetTimeStep() != m_selection.selectedTimeStep);
        extractTimeStep->SetTimeStep(m_selection.selectedTimeStep);

        // Inject the step if not done before
        if (needToInject)
        {
            modified = true;
            AbstractVisualizedData::PostProcessingStep ppStep;
            ppStep.visualizationPort = port;
            ppStep.pipelineHead = extractTimeStep;
            ppStep.pipelineTail = extractTimeStep;
            DEBUG_ONLY(const auto result =)
                m_visualization->injectPostProcessingStep(extractTimeStepPPCookie(), ppStep);
            assert(result);
        }
    }

    if (modified)
    {
        emit m_visualization->geometryChanged();
    }
}

TemporalPipelineMediator::TimeStepSelection::TimeStepSelection()
    : index{ 0 }
    , selectedTimeStep{ 0.0 }
{
}
