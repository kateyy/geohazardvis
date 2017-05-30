#include "TemporalPipelineMediator.h"

#include <algorithm>
#include <cassert>
#include <cmath>
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
#include <core/filters/TemporalDifferenceFilter.h>
#include <core/utility/macros.h>


namespace
{

const AbstractVisualizedData::StaticProcessingStepCookie & extractTimeStepPPCookie()
{
    static const auto cookie = AbstractVisualizedData::requestProcessingStepCookie();
    return cookie;
}

const AbstractVisualizedData::StaticProcessingStepCookie & temporalDifferencePPCookie()
{
    static const auto cookie = AbstractVisualizedData::requestProcessingStepCookie();
    return cookie;
}

template<typename Value_T>
size_t findNearestIndex(const std::vector<Value_T> & data, const Value_T value)
{
    const auto it = std::lower_bound(data.begin(), data.end(), value);
    if (it == data.end())
    {
        return data.size() - 1u;
    }

    return static_cast<size_t>(it - data.begin());
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
    m_selection.isTimeRange = true;
    m_selection.beginIndex = 0u;
    m_selection.beginTimeStep = m_timeSteps[0u];
    m_selection.endIndex = m_timeSteps.size() / 2;
    m_selection.endTimeStep = m_timeSteps[m_selection.endIndex];
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

    if (visualization == this->visualization())
    {
        setVisualization(nullptr);
    }

    bool modified = false;

    const auto numPorts = visualization->numberOfOutputPorts();
    for (unsigned int port = 0; port < numPorts; ++port)
    {
        const bool extractModified = visualization->erasePostProcessingStep(extractTimeStepPPCookie(), port);
        const bool diffModified = visualization->erasePostProcessingStep(temporalDifferencePPCookie(), port);
        modified = modified || extractModified || diffModified;
    }

    if (modified)
    {
        visualization->geometryChanged();
    }
}

AbstractVisualizedData * TemporalPipelineMediator::visualization()
{
    return m_visualization;
}

const AbstractVisualizedData * TemporalPipelineMediator::visualization() const
{
    return m_visualization;
}

auto TemporalPipelineMediator::timeSteps() const -> const std::vector<TimeStep_t> &
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
        && m_selection.equalsTimePoint(index, m_timeSteps[index]))
    {
        return;
    }

    m_selection.setTimePoint(index, m_timeSteps[index]);

    if (!m_visualization)
    {
        return;
    }

    passSelectionToPipeline();
}

size_t TemporalPipelineMediator::currentTimeStepIndex() const
{
    return m_selection.endIndex;
}

auto TemporalPipelineMediator::selectedTimeStep() const -> TimeStep_t
{
    return m_selection.endTimeStep;
}

void TemporalPipelineMediator::selectTemporalDifferenceByIndex(size_t beginIndex, size_t endIndex)
{
    const bool timeStepsChanged = updateTimeSteps();

    if (beginIndex >= m_timeSteps.size() || endIndex >= m_timeSteps.size())
    {
        qWarning() << "Invalid time step index:" << beginIndex << " or " << endIndex << ", have only" << m_timeSteps.size();
        return;
    }

    const SelectionInternal newSelection { true,
        beginIndex, m_timeSteps[beginIndex],
        endIndex, m_timeSteps[endIndex] };

    if (!timeStepsChanged && newSelection == m_selection)
    {
        return;
    }

    m_selection = newSelection;

    if (!m_visualization)
    {
        return;
    }

    passSelectionToPipeline();
}

auto TemporalPipelineMediator::differenceTimeSteps() const -> std::pair<TimeStep_t, TimeStep_t>
{
    return std::make_pair(m_selection.beginTimeStep, m_selection.endTimeStep);
}

std::pair<size_t, size_t> TemporalPipelineMediator::differenceTimeStepIndices() const
{
    return std::make_pair(m_selection.beginIndex, m_selection.endIndex);
}

vtkSmartPointer<vtkAlgorithm> TemporalPipelineMediator::TemporalSelection::createAlgorithm() const
{
    vtkSmartPointer<vtkAlgorithm> algorithm;

    if (!isValid)
    {
        return algorithm;
    }

    if (isTimeRange)
    {
        algorithm = vtkSmartPointer<TemporalDifferenceFilter>::New();
    }
    else
    {
        algorithm = vtkSmartPointer<ExtractTimeStep>::New();
    }

    configureAlgorithmInternal(*algorithm, nullptr);
    return algorithm;
}

void TemporalPipelineMediator::TemporalSelection::configureAlgorithm(vtkAlgorithm & algorithm,
    bool * isValidAlgorithmPtr, bool * modifiedPtr) const
{
    if (!isValid)
    {
        assert(false);
        return;
    }

    const bool isValidAlgorithm_l = isTimeRange
            ? TemporalDifferenceFilter::SafeDownCast(&algorithm) != nullptr
            : ExtractTimeStep::SafeDownCast(&algorithm) != nullptr;

    if (isValidAlgorithm_l)
    {
        configureAlgorithmInternal(algorithm, modifiedPtr);
    }
    if (isValidAlgorithmPtr)
    {
        *isValidAlgorithmPtr = isValidAlgorithm_l;
    }
}

void TemporalPipelineMediator::TemporalSelection::configureAlgorithmInternal(
    vtkAlgorithm & algorithm, bool * modifiedPtr) const
{
    assert(isValid);

    if (isTimeRange)
    {
        auto & diff = static_cast<TemporalDifferenceFilter &>(algorithm);
        if (modifiedPtr)
        {
            *modifiedPtr = *modifiedPtr
                || (diff.GetTimeStep0() != beginTimeStep)
                || (diff.GetTimeStep1() != endTimeStep);
        }
        diff.SetTimeStep0(beginTimeStep);
        diff.SetTimeStep1(endTimeStep);
    }
    else
    {
        auto & extract = static_cast<ExtractTimeStep &>(algorithm);
        if (modifiedPtr)
        {
            *modifiedPtr = *modifiedPtr || (extract.GetTimeStep() != beginTimeStep);
        }
        extract.SetTimeStep(beginTimeStep);
    }
}

auto TemporalPipelineMediator::currentPipelineSelection(AbstractVisualizedData & visualization,
    const unsigned int port) -> TemporalSelection
{
    TemporalSelection selection {
        false, false,
        std::numeric_limits<TimeStep_t>::quiet_NaN(),
        std::numeric_limits<TimeStep_t>::quiet_NaN()
    };

    auto extractStepPtr = visualization.getPostProcessingStep(extractTimeStepPPCookie(), port);
    auto differenceStepPtr = visualization.getPostProcessingStep(temporalDifferencePPCookie(), port);

    if (!extractStepPtr && !differenceStepPtr)
    {
        return selection;
    }

    if (!extractStepPtr == !differenceStepPtr)
    {
        assert(false);
        qWarning() << "Unexpected pipeline setup "
            << "(single time step extraction and temporal difference applied at the same time). "
            << "Computing temporal difference only.";
        visualization.erasePostProcessingStep(extractTimeStepPPCookie(), port);
        extractStepPtr = nullptr;
    }

    if (extractStepPtr)
    {
        auto extactTimeStep = ExtractTimeStep::SafeDownCast(extractStepPtr->pipelineHead);
        assert(extactTimeStep);
        if (extactTimeStep)
        {
            selection.isValid = true;
            selection.isTimeRange = false;
            selection.beginTimeStep = selection.endTimeStep
                = extactTimeStep->GetTimeStep();
            return selection;
        }
    }

    if (differenceStepPtr)
    {
        auto difference = TemporalDifferenceFilter::SafeDownCast(differenceStepPtr->pipelineHead);
        assert(difference);
        if (difference)
        {
            selection.isValid = true;
            selection.isTimeRange = true;
            selection.beginTimeStep = difference->GetTimeStep0();
            selection.endTimeStep = difference->GetTimeStep1();
            return selection;
        }
    }

    return selection;
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
        auto ppStep1 = m_visualization->getPostProcessingStep(extractTimeStepPPCookie(), port);
        auto ppStep2 = m_visualization->getPostProcessingStep(temporalDifferencePPCookie(), port);
        if (!ppStep1 && !ppStep2)
        {
            continue;
        }
        assert(!ppStep1 != !ppStep2);

        auto ppStep = ppStep1 ? ppStep1 : ppStep2;
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

    std::vector<TimeStep_t> pipelineTimeSteps;

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

        pipelineTimeSteps = std::vector<TimeStep_t>(ptr, ptr + numTimeSteps);

    } while (false);

    if (m_timeSteps == pipelineTimeSteps)
    {
        return false;
    }

    std::swap(m_timeSteps, pipelineTimeSteps);
    return true;
}

bool TemporalPipelineMediator::selectionFromPipeline()
{
    // Nothing to do if there is no visualization or it has not time steps.
    if (!m_visualization || m_timeSteps.empty())
    {
        return false;
    }

    bool hasPreviouSelection = false;
    SelectionInternal previousSelection;

    const auto numPorts = m_visualization->numberOfOutputPorts();

    // Check for previously injected processing steps.
    // Search only until finding the first port with temporal information.
    for (unsigned int port = 0; port < numPorts; ++port)
    {
        auto extractStepPtr = m_visualization->getPostProcessingStep(extractTimeStepPPCookie(), port);
        auto differenceStepPtr = m_visualization->getPostProcessingStep(temporalDifferencePPCookie(), port);

        if (!extractStepPtr && !differenceStepPtr)
        {
            continue;
        }
        if (!extractStepPtr == !differenceStepPtr)
        {
            assert(false);
            qWarning() << "Unexpected pipeline setup "
                << "(single time step extraction and temporal difference applied at the same time). "
                << "Computing temporal difference only.";
            m_visualization->erasePostProcessingStep(extractTimeStepPPCookie(), port);
            extractStepPtr = nullptr;
        }

        auto & step = extractStepPtr ? *extractStepPtr : *differenceStepPtr;
        assert(step.pipelineHead == step.pipelineTail);
        if (extractStepPtr)
        {
            if (auto algorithm = ExtractTimeStep::SafeDownCast(step.pipelineHead))
            {
                previousSelection.setTimePoint(0u, algorithm->GetTimeStep());
            }
        }
        else /*if (differenceStepPtr)*/
        {
            if (auto algorithm = TemporalDifferenceFilter::SafeDownCast(step.pipelineHead))
            {
                previousSelection.isTimeRange = true;
                previousSelection.beginTimeStep = algorithm->GetTimeStep0();
                previousSelection.endTimeStep = algorithm->GetTimeStep1();
            }
        }

        hasPreviouSelection = true;

        // Find nearest matching time steps based on the update time steps.
        previousSelection.beginIndex =
            findNearestIndex(m_timeSteps, previousSelection.beginTimeStep);

        if (previousSelection.beginTimeStep == previousSelection.endTimeStep)
        {
            previousSelection.endIndex = previousSelection.beginIndex;
        }
        else
        {
            previousSelection.endIndex =
                findNearestIndex(m_timeSteps, previousSelection.endTimeStep);
        }

        // Clamp update time step to actually available time steps.
        previousSelection.beginTimeStep = m_timeSteps[previousSelection.beginIndex];
        previousSelection.endTimeStep = m_timeSteps[previousSelection.endIndex];

        // Assume same temporal data on all output ports
        break;
    }

    if (!hasPreviouSelection)
    {
        return false;
    }

    m_selection = previousSelection;
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

    // Make sure that exactly one (the currently requested) processing step is injected.
    auto & ppCookieInUse = m_selection.isTimeRange
        ? temporalDifferencePPCookie()
        : extractTimeStepPPCookie();
    auto & ppCookieToRemove = !m_selection.isTimeRange
        ? temporalDifferencePPCookie()
        : extractTimeStepPPCookie();

    // Check on all visualization ports that a filter for the current setup is present, create one
    // if necessary, and configure its parameters.
    const auto numPorts = m_visualization->numberOfOutputPorts();
    const auto selection = m_selection.toTemporalSelection();
    for (unsigned int port = 0; port < numPorts; ++port)
    {
        auto ppStepPtr = m_visualization->getPostProcessingStep(ppCookieInUse, port);
        m_visualization->erasePostProcessingStep(ppCookieToRemove, port);

        assert(!ppStepPtr || ppStepPtr->pipelineHead == ppStepPtr->pipelineTail);

        vtkSmartPointer<vtkAlgorithm> newFilterToInject;
        bool isValid = false;
        if (auto previousFilter = ppStepPtr ? ppStepPtr->pipelineHead : nullptr)
        {
            selection.configureAlgorithm(*previousFilter, &isValid, &modified);
        }
        if (!isValid)
        {
            newFilterToInject = selection.createAlgorithm();
            assert(newFilterToInject);
        }

        // Inject the step if not done before
        if (newFilterToInject)
        {
            modified = true;
            AbstractVisualizedData::PostProcessingStep ppStep;
            ppStep.visualizationPort = port;
            ppStep.pipelineHead = newFilterToInject;
            ppStep.pipelineTail = newFilterToInject;
            DEBUG_ONLY(const auto result =)
                m_visualization->injectPostProcessingStep(ppCookieInUse, ppStep);
            assert(result);
        }
    }

    if (modified)
    {
        emit m_visualization->geometryChanged();
    }
}

TemporalPipelineMediator::SelectionInternal::SelectionInternal()
    : SelectionInternal(false, 0u, 0.0, 0u, 0.0)
{
}

TemporalPipelineMediator::SelectionInternal::SelectionInternal(
    bool isTimeRange,
    size_t beginIndex, TimeStep_t beginTimeStep,
    size_t endIndex, TimeStep_t endTimeStep)
    : isTimeRange{ isTimeRange }
    , beginIndex{ beginIndex }
    , beginTimeStep{ beginTimeStep }
    , endIndex{ endIndex }
    , endTimeStep{ endTimeStep }
{
}

void TemporalPipelineMediator::SelectionInternal::setTimePoint(size_t index, TimeStep_t timeStep)
{
    isTimeRange = false;
    beginIndex = endIndex = index;
    beginTimeStep = endTimeStep = timeStep;
}

bool TemporalPipelineMediator::SelectionInternal::equalsTimePoint(size_t index, TimeStep_t timeStep) const
{
    return !isTimeRange
        && beginIndex == index
        && beginTimeStep == timeStep;
}

bool TemporalPipelineMediator::SelectionInternal::operator==(const SelectionInternal & other) const
{
    return isTimeRange == other.isTimeRange
        && beginIndex == other.beginIndex
        && beginTimeStep == other.beginTimeStep
        && endIndex == other.endIndex
        && endTimeStep == other.endTimeStep;
}

bool TemporalPipelineMediator::SelectionInternal::operator!=(const SelectionInternal & other) const
{
    return !(*this == other);
}

auto TemporalPipelineMediator::SelectionInternal::toTemporalSelection() const -> TemporalSelection
{
    return TemporalSelection{ true, isTimeRange, beginTimeStep, endTimeStep };
}
