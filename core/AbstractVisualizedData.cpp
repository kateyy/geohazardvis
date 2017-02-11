#include "AbstractVisualizedData.h"

#include <type_traits>

#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkDataSet.h>
#include <vtkLookupTable.h>

#include <core/AbstractVisualizedData_private.h>
#include <core/data_objects/DataObject.h>
#include <core/utility/macros.h>


AbstractVisualizedData::AbstractVisualizedData(ContentType contentType, DataObject & dataObject)
    : QObject()
    , d_ptr{ std::make_unique<AbstractVisualizedData_private>(contentType, dataObject) }
{
    connect(&dataObject, &DataObject::dataChanged, this, &AbstractVisualizedData::geometryChanged);
}

AbstractVisualizedData::~AbstractVisualizedData()
{
    if (d_ptr->colorMapping)
    {
        d_ptr->colorMapping->unregisterVisualizedData(this);
    }
}

ContentType AbstractVisualizedData::contentType() const
{
    return d_ptr->contentType;
}

DataObject & AbstractVisualizedData::dataObject()
{
    return d_ptr->dataObject;
}

const DataObject & AbstractVisualizedData::dataObject() const
{
    return d_ptr->dataObject;
}

bool AbstractVisualizedData::isVisible() const
{
    return d_ptr->isVisible;
}

void AbstractVisualizedData::setVisible(bool visible)
{
    d_ptr->isVisible = visible;

    visibilityChangedEvent(visible);

    emit visibilityChanged(visible);
}

const DataBounds & AbstractVisualizedData::visibleBounds()
{
    if (!d_ptr->visibleBoundsAreValid())
    {
        d_ptr->validateVisibleBounds(updateVisibleBounds());
    }

    return d_ptr->visibleBounds();
}

ColorMapping & AbstractVisualizedData::colorMapping()
{
    if (!d_ptr->colorMapping)
    {
        d_ptr->colorMapping = std::make_unique<ColorMapping>();
        d_ptr->colorMapping->registerVisualizedData(this);
        setupColorMapping(*d_ptr->colorMapping);
    }

    return *d_ptr->colorMapping;
}

void AbstractVisualizedData::setScalarsForColorMapping(ColorMappingData * scalars)
{
    if (scalars == d_ptr->colorMappingData)
    {
        return;
    }

    d_ptr->colorMappingData = scalars;

    scalarsForColorMappingChangedEvent();
}

void AbstractVisualizedData::setColorMappingGradient(vtkScalarsToColors * gradient)
{
    if (d_ptr->gradient == gradient)
    {
        return;
    }

    d_ptr->gradient = gradient;

    colorMappingGradientChangedEvent();
}

unsigned int AbstractVisualizedData::numberOfOutputPorts() const
{
    return 1;
}

vtkAlgorithmOutput * AbstractVisualizedData::processedOutputPort(const unsigned int port)
{
    if (port >= numberOfOutputPorts())
    {
        assert(false);
        return nullptr;
    }

    if (d_ptr->pipelineEndpointsPerPort.size() <= port
        || d_ptr->pipelineEndpointsPerPort[port] == nullptr)
    {
        updatePipeline(port);
    }

    assert(d_ptr->pipelineEndpointsPerPort[port]);
    return d_ptr->pipelineEndpointsPerPort[port];
}

vtkDataSet * AbstractVisualizedData::processedOutputDataSet(unsigned int port)
{
    auto alg = processedOutputPort(port)->GetProducer();
    alg->Update();
    return vtkDataSet::SafeDownCast(alg->GetOutputDataObject(0));
}

vtkAlgorithmOutput * AbstractVisualizedData::processedOutputPortInternal(unsigned int DEBUG_ONLY(port))
{
    assert(port == 0);

    return dataObject().processedOutputPort();
}

std::pair<bool, unsigned int> AbstractVisualizedData::injectPostProcessingStep(const PostProcessingStep & postProcessingStep)
{
    d_ptr->postProcessingStepsPerPort.resize(postProcessingStep.visualizationPort + 1u);
    auto & dynamicStepsForPort =
        d_ptr->postProcessingStepsPerPort[postProcessingStep.visualizationPort].dynamicStepTypes;

    const auto newId = d_ptr->getNextProcessingStepId();

    dynamicStepsForPort.emplace_back(newId, postProcessingStep);

    updatePipeline(postProcessingStep.visualizationPort);

    return std::make_pair(true, newId);
}

bool AbstractVisualizedData::erasePostProcessingStep(const unsigned int id)
{
    for (size_t port = 0; port < d_ptr->postProcessingStepsPerPort.size(); ++port)
    {
        auto & dynamicStepsForPort = d_ptr->postProcessingStepsPerPort[port].dynamicStepTypes;
        const auto it = std::find_if(dynamicStepsForPort.begin(), dynamicStepsForPort.end(),
            [id] (const std::pair<unsigned int, PostProcessingStep> & step)
        {
            return step.first == id;
        });

        if (it == dynamicStepsForPort.end())
        {
            continue;
        }

        dynamicStepsForPort.erase(it);
        updatePipeline(static_cast<int>(port));
        d_ptr->releaseProcessingStepId(id);

        return true;
    }

    return false;
}

void AbstractVisualizedData::updatePipeline(const unsigned int port)
{
    auto upstream = processedOutputPortInternal(port);
    assert(upstream);

    d_ptr->pipelineEndpointsPerPort.resize(port + 1);

    // No post processing pipeline for this port:
    if (d_ptr->postProcessingStepsPerPort.size() <= port)
    {
        d_ptr->pipelineEndpointsPerPort[port] = upstream;
        return;
    }

    vtkSmartPointer<vtkAlgorithmOutput> currentUpstream = upstream;

    const auto & ppSteps = d_ptr->postProcessingStepsPerPort[port];
    // Simple convention for now: static first, dynamic afterwards.
    // Now priority, position, dependency for now
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

    d_ptr->pipelineEndpointsPerPort[port] = currentUpstream;
}

AbstractVisualizedData::StaticProcessingStepCookie AbstractVisualizedData::requestProcessingStepCookie()
{
    static std::remove_const_t<decltype(StaticProcessingStepCookie::id)> nextId = {};

    return StaticProcessingStepCookie{ nextId++ };
}

bool AbstractVisualizedData::injectPostProcessingStep(
    const StaticProcessingStepCookie & cookie,
    const PostProcessingStep & postProcessingStep)
{
    d_ptr->postProcessingStepsPerPort.resize(postProcessingStep.visualizationPort + 1u);
    auto & staticStepsForPort =
        d_ptr->postProcessingStepsPerPort[postProcessingStep.visualizationPort].staticStepTypes;

    const auto result = staticStepsForPort.emplace(cookie, std::make_unique<PostProcessingStep>(postProcessingStep));
    if (!result.second)
    {
        // cookie is already in use, abort the insertion
        return false;
    }

    updatePipeline(postProcessingStep.visualizationPort);

    return true;
}

bool AbstractVisualizedData::hasPostProcessingStep(const StaticProcessingStepCookie & cookie, unsigned int port) const
{
    if (d_ptr->postProcessingStepsPerPort.size() <= port)
    {
        return false;
    }

    const auto & staticStepsForPort = d_ptr->postProcessingStepsPerPort[port].staticStepTypes;
    return staticStepsForPort.find(cookie) != staticStepsForPort.end();
}

AbstractVisualizedData::PostProcessingStep * AbstractVisualizedData::getPostProcessingStep(
    const StaticProcessingStepCookie & cookie, unsigned int port)
{
    if (d_ptr->postProcessingStepsPerPort.size() <= port)
    {
        return nullptr;
    }

    auto & staticStepsForPort = d_ptr->postProcessingStepsPerPort[port].staticStepTypes;
    auto it = staticStepsForPort.find(cookie);
    if (it == staticStepsForPort.end())
    {
        return nullptr;
    }

    return it->second.get();
}

bool AbstractVisualizedData::erasePostProcessingStep(const StaticProcessingStepCookie & cookie, unsigned int port)
{
    if (d_ptr->postProcessingStepsPerPort.size() <= port)
    {
        return false;
    }

    auto & staticStepsForPort = d_ptr->postProcessingStepsPerPort[port].staticStepTypes;
    return staticStepsForPort.erase(cookie) != 0u;
}

void AbstractVisualizedData::setupInformation(vtkInformation & information, AbstractVisualizedData & visualization)
{
    auto & dataObject = visualization.dataObject();
    DataObject::storePointer(information, &dataObject);
    DataObject::storeName(information, dataObject);
    AbstractVisualizedData::storePointer(information, &visualization);
}

AbstractVisualizedData * AbstractVisualizedData::readPointer(vtkInformation & information)
{
    static_assert(sizeof(int*) == sizeof(DataObject*), "");

    if (information.Has(AbstractVisualizedData_private::VISUALIZED_DATA()))
    {
        assert(information.Length(AbstractVisualizedData_private::VISUALIZED_DATA()) == 1);
        return reinterpret_cast<AbstractVisualizedData *>(information.Get(AbstractVisualizedData_private::VISUALIZED_DATA()));
    }

    return nullptr;
}

void AbstractVisualizedData::storePointer(vtkInformation & information, AbstractVisualizedData * visualization)
{
    information.Set(AbstractVisualizedData_private::VISUALIZED_DATA(), reinterpret_cast<int *>(visualization), 1);
}

void AbstractVisualizedData::setupColorMapping(ColorMapping & /*colorMapping*/)
{
}

void AbstractVisualizedData::visibilityChangedEvent(bool /*visible*/)
{
}

void AbstractVisualizedData::scalarsForColorMappingChangedEvent()
{
}

void AbstractVisualizedData::colorMappingGradientChangedEvent()
{
}

ColorMappingData * AbstractVisualizedData::currentColorMappingData()
{
    return d_ptr->colorMappingData;
}

vtkScalarsToColors * AbstractVisualizedData::currentColorMappingGradient()
{
    return d_ptr->gradient;
}

void AbstractVisualizedData::invalidateVisibleBounds()
{
    d_ptr->invalidateVisibleBounds();
}
