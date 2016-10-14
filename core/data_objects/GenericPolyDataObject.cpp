#include "GenericPolyDataObject.h"

#include <cassert>

#include <QDebug>

#include <vtkAlgorithmOutput.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <core/CoordinateSystems.h>
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
    const CoordinateSystemSpecification & coordsSpec)
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
        if (spec.isValid(false) && (spec == coordsSpec))
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

vtkSmartPointer<vtkAlgorithm> GenericPolyDataObject::createTransformPipeline(const CoordinateSystemSpecification & toSystem, vtkAlgorithmOutput * pipelineUpstream) const
{
    if (coordinateSystem().geographicSystem != "WGS 84"
        || toSystem.geographicSystem != "WGS 84"
        || coordinateSystem().globalMetricSystem != "UTM"
        || toSystem.globalMetricSystem != "UTM"
        || !coordinateSystem().isReferencePointValid())
    {
        return{};
    }

    // check if there are stored point coordinates
    pipelineUpstream->GetProducer()->Update();
    auto upstreamPoly = vtkPolyData::SafeDownCast(pipelineUpstream->GetProducer()->GetOutputDataObject(0));
    if (upstreamPoly)
    {
        if (auto storedCoords = findCoordinatesArray(*upstreamPoly, toSystem))
        {
            auto && spec = ReferencedCoordinateSystemSpecification::fromInformation(
                *storedCoords->GetInformation());

            auto assignCoords = vtkSmartPointer<AssignPointAttributeToCoordinatesFilter>::New();
            assignCoords->SetInputConnection(pipelineUpstream);
            assignCoords->SetAttributeArrayToAssign(storedCoords->GetName());

            auto setCoordsSpec = vtkSmartPointer<SetCoordinateSystemInformationFilter>::New();
            setCoordsSpec->SetCoordinateSystemSpec(spec);
            setCoordsSpec->SetInputConnection(assignCoords->GetOutputPort());

            return setCoordsSpec;
        }

        if (toSystem.type == CoordinateSystemType::metricLocal)
        {
            auto fromGlobalSystem = coordinateSystem();
            fromGlobalSystem.type = CoordinateSystemType::metricGlobal;
            auto globalCoords = coordinateSystem().type == CoordinateSystemType::metricGlobal
                ? upstreamPoly->GetPoints()->GetData()
                : findCoordinatesArray(*upstreamPoly, fromGlobalSystem);

            if (globalCoords)
            {
                DataExtent<double, 2> globalBounds;
                globalCoords->GetRange(&globalBounds.data()[0], 0);
                globalCoords->GetRange(&globalBounds.data()[2], 1);

                const auto refPointGlobal = globalBounds.min() 
                    + globalBounds.componentSize() * fromGlobalSystem.referencePointLocalRelative;

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

                auto && spec = ReferencedCoordinateSystemSpecification::fromInformation(
                    *globalCoords->GetInformation());

                auto setCoordsSpec = vtkSmartPointer<SetCoordinateSystemInformationFilter>::New();
                setCoordsSpec->SetCoordinateSystemSpec(spec);
                setCoordsSpec->SetInputConnection(transformFilter->GetOutputPort());
                
                return setCoordsSpec;
            }
        }
    }

    if (coordinateSystem().type == CoordinateSystemType::geographic
        && toSystem.type == CoordinateSystemType::metricLocal)
    {
        auto filter = vtkSmartPointer<SimplePolyGeoCoordinateTransformFilter>::New();
        filter->SetInputConnection(pipelineUpstream);
        filter->SetTargetCoordinateSystem(SimplePolyGeoCoordinateTransformFilter::LOCAL_METRIC);
        return filter;
    }

    if (coordinateSystem().type == CoordinateSystemType::metricLocal
        && toSystem.type == CoordinateSystemType::geographic)
    {
        auto filter = vtkSmartPointer<SimplePolyGeoCoordinateTransformFilter>::New();
        filter->SetInputConnection(pipelineUpstream);
        filter->SetTargetCoordinateSystem(SimplePolyGeoCoordinateTransformFilter::GLOBAL_GEOGRAPHIC);
        return filter;
    }

    return{};
}
