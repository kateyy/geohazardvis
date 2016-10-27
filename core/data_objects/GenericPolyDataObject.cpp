#include "GenericPolyDataObject.h"

#include <cassert>

#include <QDebug>

#include <vtkAlgorithmOutput.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <core/CoordinateSystems.h>
#include <core/types.h>
#include <core/data_objects/DataObject_private.h>
#include <core/data_objects/PointCloudDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/filters/AssignPointAttributeToCoordinatesFilter.h>
#include <core/filters/SetCoordinateSystemInformationFilter.h>
#include <core/filters/SimplePolyGeoCoordinateTransformFilter.h>
#include <core/utility/vtkvectorhelper.h>


namespace
{


vtkDataArray * findCoordinatesArray(
    vtkPointSet & dataSet,
    const CoordinateSystemType & coordsType)
{
    auto & pointData = *dataSet.GetPointData();
    const auto numArrays = pointData.GetNumberOfArrays();
    for (int i = 0; i < numArrays; ++i)
    {
        auto array = pointData.GetArray(i);
        if (!array || !array->GetName())
        {
            continue;
        }
        const auto spec = CoordinateSystemSpecification::fromInformation(*array->GetInformation());
        if (spec.isValid() && (spec.type == coordsType))
        {
            return array;
        }
    }

    return nullptr;
}


}

GenericPolyDataObject::GenericPolyDataObject(const QString & name, vtkPolyData & dataSet)
    : CoordinateTransformableDataObject(name, &dataSet)
{
}

GenericPolyDataObject::~GenericPolyDataObject() = default;

bool GenericPolyDataObject::is3D() const
{
    return true;
}

vtkPolyData & GenericPolyDataObject::polyDataSet()
{
    auto ds = dataSet();
    assert(dynamic_cast<vtkPolyData *>(ds));
    return static_cast<vtkPolyData &>(*ds);
}

const vtkPolyData & GenericPolyDataObject::polyDataSet() const
{
    auto ds = dataSet();
    assert(dynamic_cast<const vtkPolyData *>(ds));
    return static_cast<const vtkPolyData &>(*ds);
}

std::unique_ptr<GenericPolyDataObject> GenericPolyDataObject::createInstance(const QString & name, vtkPolyData & dataSet)
{
    const bool hasLines = 0 != dataSet.GetNumberOfLines();
    const bool hasStrips = 0 != dataSet.GetNumberOfStrips();

    if (hasStrips || hasLines)
    {
        qWarning() << "GenericPolyDataObject: unsupported cell types in data set """ + name + """.";
        return{};
    }

    const bool hasVerts = 0 != dataSet.GetNumberOfVerts();
    const bool hasPolys = 0 != dataSet.GetNumberOfPolys();

    if (!hasVerts && !hasPolys)
    {
        qWarning() << "GenericPolyDataObject: no supported cell types in data set """ + name + """.";
        return{};
    }

    if (hasVerts && hasPolys)
    {
        qWarning() << "GenericPolyDataObject: mixed cell types are not supported (""" + name + """).";
        return{};
    }

    if (dataSet.GetNumberOfCells() !=
        dataSet.GetVerts()->GetNumberOfCells()
        + dataSet.GetPolys()->GetNumberOfCells()
        + dataSet.GetStrips()->GetNumberOfCells()
        + dataSet.GetLines()->GetNumberOfCells())
    {
        qWarning() << "GenericPolyDataObject: data set contains unknown cell type (""" + name + """).";
        return{};
    }
    
    if (hasPolys)
    {
        return std::make_unique<PolyDataObject>(name, dataSet);
    }

    if (hasVerts)
    {
        return std::make_unique<PointCloudDataObject>(name, dataSet);
    }

    assert(false);

    return{};
}

double GenericPolyDataObject::pointCoordinateComponent(vtkIdType pointId, int component, bool * validIdPtr)
{
    auto & points = *polyDataSet().GetPoints()->GetData();

    const auto isValid = pointId < points.GetNumberOfTuples()
        && component < points.GetNumberOfComponents();

    if (validIdPtr)
    {
        *validIdPtr = isValid;
    }

    if (!isValid)
    {
        return{};
    }

    return points.GetComponent(pointId, component);
}

bool GenericPolyDataObject::setPointCoordinateComponent(vtkIdType pointId, int component, double value)
{
    auto & points = *polyDataSet().GetPoints()->GetData();

    if (pointId >= points.GetNumberOfTuples()
        || component >= points.GetNumberOfComponents())
    {
        return false;
    }

    points.SetComponent(pointId, component, value);
    polyDataSet().Modified();

    return true;
}

bool GenericPolyDataObject::checkIfStructureChanged()
{
    const bool superclassResult = CoordinateTransformableDataObject::checkIfStructureChanged();

    return superclassResult || dPtr().m_inCopyStructure;
}

vtkSmartPointer<vtkAlgorithm> GenericPolyDataObject::createTransformPipeline(
    const CoordinateSystemSpecification & toSystem,
    vtkAlgorithmOutput * pipelineUpstream) const
{
    // Limited coordinate system support...
    if (coordinateSystem().geographicSystem != "WGS 84"
        || toSystem.geographicSystem != "WGS 84"
        || coordinateSystem().globalMetricSystem != "UTM"
        || toSystem.globalMetricSystem != "UTM")
    {
        return{};
    }

    if (!coordinateSystem().isReferencePointValid())
    {   // If anything else than unit conversions is requested, a reference point is required.
        auto equalExceptUnitCheck = toSystem;
        equalExceptUnitCheck.unitOfMeasurement = coordinateSystem().unitOfMeasurement;
        if (equalExceptUnitCheck != coordinateSystem())
        {
            return{};
        }
    }

    vtkSmartPointer<vtkAlgorithm> localPipelineUpstream;

    // check if there are stored point coordinates
    pipelineUpstream->GetProducer()->Update();
    auto upstreamPoly = vtkPolyData::SafeDownCast(pipelineUpstream->GetProducer()->GetOutputDataObject(0));
    if (upstreamPoly)
    {
        // Check if there are stored coordinates in the requested system type.
        auto storedCoordsByType = findCoordinatesArray(*upstreamPoly, toSystem.type);

        if (storedCoordsByType)
        {
            auto && storedCoordsByTypeSpec = ReferencedCoordinateSystemSpecification::fromInformation(
                *storedCoordsByType->GetInformation());
            auto assignCoords = vtkSmartPointer<AssignPointAttributeToCoordinatesFilter>::New();
            assignCoords->SetInputConnection(pipelineUpstream);
            assignCoords->SetAttributeArrayToAssign(storedCoordsByType->GetName());

            auto setCoordsSpec = vtkSmartPointer<SetCoordinateSystemInformationFilter>::New();
            setCoordsSpec->SetCoordinateSystemSpec(storedCoordsByTypeSpec);
            setCoordsSpec->SetInputConnection(assignCoords->GetOutputPort());

            // already in the target system?
            if (storedCoordsByTypeSpec == toSystem)
            {
                return setCoordsSpec;
            }
            localPipelineUpstream = setCoordsSpec;
        }
        // Just a metric global->local transformation based on stored global metric coordinates?
        // This can be done here.
        else if (toSystem.type == CoordinateSystemType::metricLocal)
        {
            if (auto globalCoords = coordinateSystem().type == CoordinateSystemType::metricGlobal
                ? upstreamPoly->GetPoints()->GetData()
                : findCoordinatesArray(*upstreamPoly, CoordinateSystemType::metricGlobal))
            {
                DataExtent<double, 2> globalBounds;
                globalCoords->GetRange(&globalBounds.data()[0], 0);
                globalCoords->GetRange(&globalBounds.data()[2], 1);

                const auto globalCoordsSpec = coordinateSystem().type == CoordinateSystemType::metricGlobal
                    ? coordinateSystem()
                    : ReferencedCoordinateSystemSpecification::fromInformation(*globalCoords->GetInformation());

                const auto refPointGlobal = globalBounds.min()
                    + globalBounds.componentSize() * globalCoordsSpec.referencePointLocalRelative;

                auto transform = vtkSmartPointer<vtkTransform>::New();
                transform->Translate(convertTo<3>(-refPointGlobal).GetData());

                vtkSmartPointer<vtkAlgorithm> upstream = pipelineUpstream->GetProducer();

                if (coordinateSystem().type != CoordinateSystemType::metricGlobal)
                {
                    auto assignGlobalCoords = vtkSmartPointer<AssignPointAttributeToCoordinatesFilter>::New();
                    assignGlobalCoords->SetInputConnection(pipelineUpstream);
                    assignGlobalCoords->SetAttributeArrayToAssign(globalCoords->GetName());
                    upstream = assignGlobalCoords;
                }

                auto transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
                transformFilter->SetTransform(transform);
                transformFilter->SetInputConnection(upstream->GetOutputPort());

                auto localCoordsSpec = globalCoordsSpec;
                localCoordsSpec.type = CoordinateSystemType::metricLocal;

                auto setCoordsSpec = vtkSmartPointer<SetCoordinateSystemInformationFilter>::New();
                setCoordsSpec->SetCoordinateSystemSpec(localCoordsSpec);
                setCoordsSpec->SetInputConnection(transformFilter->GetOutputPort());

                // already in the target system?
                if (localCoordsSpec == toSystem)
                {
                    return setCoordsSpec;
                }
                localPipelineUpstream = setCoordsSpec;
            }
        }
    }

    // If the target system could not be generated using stored coordinates, support from the
    // transform filter is required.
    if (!localPipelineUpstream)
    {
        static const QList<CoordinateSystemType::Value> transformableTypes
        { CoordinateSystemType::geographic, CoordinateSystemType::metricLocal };

        if ((coordinateSystem().type != toSystem.type)
            && (!transformableTypes.contains(coordinateSystem().type)
                || !transformableTypes.contains(toSystem.type)))
        {
            return{};
        }
        else
        {
            // Just pass the work to the SimplePolyGeoCoordinateTransformFilter
            localPipelineUpstream = pipelineUpstream->GetProducer();
        }
    }

    // This transforms between local metric coordinates and geographic coordinates,
    // and/or transforms metric coordinate units.
    auto filter = vtkSmartPointer<SimplePolyGeoCoordinateTransformFilter>::New();
    filter->SetInputConnection(localPipelineUpstream->GetOutputPort());
    filter->SetTargetCoordinateSystemType(toSystem.type);
    return filter;
}
