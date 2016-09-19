#include "CoordinateTransformableDataObject.h"

#include <vtkAlgorithmOutput.h>
#include <vtkDataSet.h>
#include <vtkPassThrough.h>


CoordinateTransformableDataObject::CoordinateTransformableDataObject(
    const QString & name, vtkDataSet * dataSet)
    : DataObject(name, dataSet)
{
    if (dataSet)
    {
        m_spec.readFromInformation(*dataSet->GetInformation());
    }
}

CoordinateTransformableDataObject::~CoordinateTransformableDataObject() = default;

const ReferencedCoordinateSystemSpecification & CoordinateTransformableDataObject::coordinateSystem() const
{
    return m_spec;
}

vtkSmartPointer<vtkAlgorithm> CoordinateTransformableDataObject::createTransformPipeline(const CoordinateSystemSpecification & /*toSystem*/, vtkAlgorithmOutput * /*pipelineUpstream*/) const
{
    // nothing supported by default
    return nullptr;
}

ConversionCheckResult CoordinateTransformableDataObject::canTransformToInternal(
    const CoordinateSystemSpecification & toSystem) const
{
    // check for missing/superfluous parameters
    if (!toSystem.isValid(false))
    {
        return ConversionCheckResult::invalidParameters();
    }

    // check for missing specification for the data set
    if (!m_spec.isValid(false))
    {
        return ConversionCheckResult::missingInformation();
    }

    // same as data set
    if (m_spec == toSystem)
    {
        // this is handled by a pass-through
        return ConversionCheckResult::okay();
    }

    // check the cases where a reference point is required
    const bool referencePointIsDefined = m_spec.isReferencePointValid();

    if (!referencePointIsDefined)
    {
        do
        {
            // global<->local transformation in one system
            if (m_spec.geographicSystem == toSystem.geographicSystem
                && m_spec.globalMetricSystem == toSystem.globalMetricSystem)
            {
                // (my type != new type)

                return ConversionCheckResult::missingInformation();
            }

            // conversion between global systems -> no reference point needed
            if (m_spec.type != CoordinateSystemType::metricLocal
                && toSystem.type != CoordinateSystemType::metricLocal)
            {
                break;
            }

        } while (false);

        // can't find a reason not to require it
        return ConversionCheckResult::missingInformation();
    }

    return ConversionCheckResult::okay();
}

ConversionCheckResult CoordinateTransformableDataObject::canTransformTo(
    const CoordinateSystemSpecification & toSystem)
{
    const auto constCheckResult = canTransformToInternal(toSystem);
    if (!constCheckResult)
    {
        return constCheckResult;
    }

    // now check if subclasses implement the required filter
    if (!transformFilter(toSystem))
    {
        return ConversionCheckResult::unsupported();
    }

    return ConversionCheckResult::okay();
}

bool CoordinateTransformableDataObject::specifyCoordinateSystem(
    const ReferencedCoordinateSystemSpecification & coordinateSystem)
{
    if (!coordinateSystem.isValid(true))
    {
        return false;
    }

    if (m_spec == coordinateSystem)
    {
        return true;
    }

    m_spec = coordinateSystem;
    if (auto info = informationStorage())
    {
        m_spec.writeToInformation(*info);
    }

    emit coordinateSystemChanged();

    m_pipelines.clear();

    return true;
}

vtkAlgorithmOutput * CoordinateTransformableDataObject::coordinateTransformedOutputPort(
    const CoordinateSystemSpecification & spec)
{
    if (auto filter = transformFilter(spec))
    {
        return filter->GetOutputPort(0);
    }

    return nullptr;
}

vtkSmartPointer<vtkDataSet> CoordinateTransformableDataObject::coordinateTransformedDataSet(const CoordinateSystemSpecification & spec)
{
    auto port = coordinateTransformedOutputPort(spec);
    if (!port)
    {
        return nullptr;
    }

    port->GetProducer()->Update();
    return vtkDataSet::SafeDownCast(port->GetProducer()->GetOutputDataObject(0));
}

vtkInformation * CoordinateTransformableDataObject::informationStorage()
{
    if (auto d = dataSet())
    {
        return d->GetInformation();
    }

    auto port = processedOutputPort();
    if (!port)
    {
        return nullptr;
    }

    auto producer = port->GetProducer();
    if (!producer)
    {
        return nullptr;
    }

    producer->UpdateInformation();
    auto dataObject = producer->GetOutputDataObject(0);
    if (!dataObject)
    {
        return nullptr;
    }

    return dataObject->GetInformation();
}

vtkAlgorithm * CoordinateTransformableDataObject::passThrough()
{
    if (!m_passThrough)
    {
        m_passThrough = vtkSmartPointer<vtkPassThrough>::New();
        m_passThrough->SetInputConnection(processedOutputPort());
    }

    return m_passThrough;
}

vtkAlgorithm * CoordinateTransformableDataObject::transformFilter(const CoordinateSystemSpecification & toSystem)
{
    auto currentAlgorithm = 
        [this, &toSystem] () -> vtkAlgorithm * {

        const auto typeIt = m_pipelines.find(toSystem.type);
        if (typeIt == m_pipelines.end())
        {
            return nullptr;
        }
        const auto geoIt = typeIt->find(toSystem.geographicSystem);
        if (geoIt == typeIt->end())
        {
            return nullptr;
        }
        const auto metricIt = geoIt->find(toSystem.globalMetricSystem);
        if (metricIt == geoIt->end())
        {
            return nullptr;
        }

        return *metricIt;
    }();

    if (currentAlgorithm)
    {
        return currentAlgorithm;
    }

    if (!canTransformToInternal(toSystem))
    {
        return nullptr;
    }

    if (toSystem == m_spec)
    {
        return passThrough();
    }

    auto newAlgorithm = createTransformPipeline(toSystem, processedOutputPort());
    m_pipelines[toSystem.type][toSystem.geographicSystem][toSystem.globalMetricSystem] = newAlgorithm;

    return newAlgorithm;
}
