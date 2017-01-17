#include "AbstractVisualizedData.h"

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

vtkAlgorithmOutput * AbstractVisualizedData::processedOutputPortInternal(unsigned int port)
{
    assert(port == 0);

    return dataObject().processedOutputPort();
}

std::pair<bool, unsigned int> AbstractVisualizedData::injectPostProcessingStep(const PostProcessingStep & postProcessingStep)
{
    const unsigned int port = postProcessingStep.visualizationPort;
    if (port < 0)
    {
        std::make_pair(false, 0);
    }

    d_ptr->postProcessingStepsPerPort.resize(static_cast<size_t>(port + 1));
    auto & stepsForPort = d_ptr->postProcessingStepsPerPort[static_cast<size_t>(port)];

    const auto newId = d_ptr->getNextProcessingStepId();

    stepsForPort.emplace_back(newId, postProcessingStep);

    updatePipeline(port);

    return std::make_pair(true, newId);
}

bool AbstractVisualizedData::erasePostProcessingStep(const unsigned int id)
{
    for (size_t port = 0; port < d_ptr->postProcessingStepsPerPort.size(); ++port)
    {
        auto & stepsForPort = d_ptr->postProcessingStepsPerPort[port];
        const auto it = std::find_if(stepsForPort.begin(), stepsForPort.end(),
            [id] (const std::pair<unsigned int, PostProcessingStep> & step)
        {
            return step.first == id;
        });

        if (it == stepsForPort.end())
        {
            continue;
        }

        stepsForPort.erase(it);
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
    for (const auto & step : ppSteps)
    {
        step.second.pipelineHead->SetInputConnection(currentUpstream);
        currentUpstream = step.second.pipelineTail->GetOutputPort();
    }

    d_ptr->pipelineEndpointsPerPort[port] = currentUpstream;
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
