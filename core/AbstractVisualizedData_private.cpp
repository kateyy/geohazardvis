#include "AbstractVisualizedData_private.h"

#include <cassert>

#include <vtkAlgorithmOutput.h>
#include <vtkPassThrough.h>


vtkInformationKeyMacro(AbstractVisualizedData_private, VISUALIZED_DATA, IntegerPointer);

vtkAlgorithm * AbstractVisualizedData_private::pipelineEndpointsAtPort(const unsigned int port)
{
    m_pipelineEndpointsAtPort.resize(
        std::max(m_pipelineEndpointsAtPort.size(), static_cast<size_t>(port + 1u)));
    auto & endpoint = m_pipelineEndpointsAtPort[port];
    if (!endpoint)
    {
        // Initial hard-coded pipeline. When extending it using injectPostProcessingStep,
        // updatePipeline() will set a different input here.
        endpoint = vtkSmartPointer<vtkPassThrough>::New();
        endpoint->SetInputConnection(q_ptr.processedOutputPortInternal(port));
    }
    return endpoint;
}

void AbstractVisualizedData_private::updatePipeline(const unsigned int port)
{
    auto upstream = q_ptr.processedOutputPortInternal(port);
    assert(upstream);

    vtkSmartPointer<vtkAlgorithmOutput> currentUpstream = upstream;

    const auto & ppSteps = postProcessingStepsPerPort[port];
    // Simple convention for now: static first, dynamic afterwards.
    // No priority, position, dependency for now
    for (const auto & step : ppSteps.staticStepTypes)
    {
        step.second->pipelineHead->SetInputConnection(currentUpstream);
        currentUpstream = step.second->pipelineTail->GetOutputPort();
    }
    // NOTE: steps are inserted in the order they are provided by the static cookie registration.
    // This order depends on many factors, so take care not to built any dependencies, as long as
    // they are not supported.
    for (const auto & step : ppSteps.dynamicStepTypes)
    {
        step.second.pipelineHead->SetInputConnection(currentUpstream);
        currentUpstream = step.second.pipelineTail->GetOutputPort();
    }

    pipelineEndpointsAtPort(port)->SetInputConnection(currentUpstream);
}
