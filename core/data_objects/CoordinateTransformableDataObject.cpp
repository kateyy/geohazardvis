#include "CoordinateTransformableDataObject.h"

#include <QDebug>

#include <vtkAlgorithmOutput.h>
#include <vtkDataSet.h>
#include <vtkExecutive.h>
#include <vtkPassThrough.h>

#include <core/filters/SetCoordinateSystemInformationFilter.h>


CoordinateTransformableDataObject::CoordinateTransformableDataObject(
    const QString & name, vtkDataSet * dataSet)
    : DataObject(name, dataSet)
    , m_coordsSetter{ vtkSmartPointer<SetCoordinateSystemInformationFilter>::New() }
{
    if (!dataSet)
    {
        return;
    }

    m_coordsSetter->SetInputConnection(DataObject::processedOutputPortInternal());

    const auto spec = ReferencedCoordinateSystemSpecification::fromFieldData(*dataSet->GetFieldData());
    if (spec.isValid())
    {
        m_coordsSetter->SetCoordinateSystemSpec(spec);
    }
}

CoordinateTransformableDataObject::~CoordinateTransformableDataObject() = default;

const ReferencedCoordinateSystemSpecification & CoordinateTransformableDataObject::coordinateSystem() const
{
    return m_coordsSetter->GetCoordinateSystemSpec();
}

vtkSmartPointer<vtkAlgorithm> CoordinateTransformableDataObject::createTransformPipeline(const CoordinateSystemSpecification & /*toSystem*/, vtkAlgorithmOutput * /*pipelineUpstream*/) const
{
    // nothing supported by default
    return nullptr;
}

ConversionCheckResult CoordinateTransformableDataObject::canTransformToInternal(
    const CoordinateSystemSpecification & toSystem) const
{
    // check for missing parameters
    if (!toSystem.isValid())
    {
        if (toSystem.isUnspecified())
        {   // pass-trough is okay here
            return ConversionCheckResult::okay();
        }

        return ConversionCheckResult::invalidParameters();
    }

    // check for missing specification for the data set
    if (!coordinateSystem().isValid())
    {
        return ConversionCheckResult::missingInformation();
    }

    // same as data set
    if (coordinateSystem() == toSystem)
    {
        // this is handled by a pass-through
        return ConversionCheckResult::okay();
    }

    // Coordinate unit conversions generally don't require further information
    {
        auto equalExceptUnitCheck = toSystem;
        equalExceptUnitCheck.unitOfMeasurement = coordinateSystem().unitOfMeasurement;
        if (equalExceptUnitCheck == coordinateSystem())
        {
            return ConversionCheckResult::okay();
        }
    }

    // Check cases where a reference point is required but not specified.
    if (!coordinateSystem().isReferencePointValid())
    {
        // global<->local transformation in one system always require a reference point.
        if (coordinateSystem().type != toSystem.type
            && coordinateSystem().geographicSystem == toSystem.geographicSystem
            && coordinateSystem().globalMetricSystem == toSystem.globalMetricSystem)
        {
            return ConversionCheckResult::missingInformation();
        }
    }

    return ConversionCheckResult::okay();
}

ConversionCheckResult CoordinateTransformableDataObject::canTransformTo(
    const CoordinateSystemSpecification & toSystem)
{
    const auto internalCheckResult = canTransformToInternal(toSystem);
    if (!internalCheckResult)
    {
        return internalCheckResult;
    }

    // now check if subclasses implement the required filter
    if (!transformFilter(toSystem))
    {
        return ConversionCheckResult::unsupported();
    }

    return ConversionCheckResult::okay();
}

bool CoordinateTransformableDataObject::specifyCoordinateSystem(
    const ReferencedCoordinateSystemSpecification & spec)
{
    if (!spec.isValid() && !spec.isUnspecified())
    {
        return false;
    }

    if (this->coordinateSystem() == spec)
    {
        return true;
    }

    const ScopedEventDeferral deferral(*this);

    m_coordsSetter->SetCoordinateSystemSpec(spec);

    // Make the information available to the output of dataSet().
    // This is required where using processedOutputPort() or processedDataSet() is not appropriate,
    // e.g., in the Exporter.
    if (auto ds = dataSet())
    {
        m_coordsSetter->GetCoordinateSystemSpec().writeToFieldData(*ds->GetFieldData());
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
    if (!port->GetProducer()->GetExecutive()->Update())
    {
        return nullptr;
    }
    return vtkDataSet::SafeDownCast(port->GetProducer()->GetOutputDataObject(0));
}

vtkAlgorithmOutput * CoordinateTransformableDataObject::processedOutputPortInternal()
{
    return m_coordsSetter->GetOutputPort();
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

    if (toSystem == coordinateSystem()
        || toSystem.isUnspecified())    // allow unspecified if requested
    {
        return passThrough();
    }

    auto newAlgorithm = createTransformPipeline(toSystem, processedOutputPort());
    m_pipelines[toSystem.type][toSystem.geographicSystem][toSystem.globalMetricSystem] = newAlgorithm;

    return newAlgorithm;
}
