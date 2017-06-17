#include "GeographicTransformationUtil.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <numeric>

#include <vtkDoubleArray.h>
#include <vtkExecutive.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <core/filters/GeographicTransformationFilter.h>
#include <core/filters/SetCoordinateSystemInformationFilter.h>


GeographicTransformationUtil::GeographicTransformationUtil()
    : m_coordinates{ vtkSmartPointer<vtkDoubleArray>::New() }
    , m_dataSet{ vtkSmartPointer<vtkPolyData>::New() }
    , m_spec{ vtkSmartPointer<SetCoordinateSystemInformationFilter>::New() }
    , m_transform{ vtkSmartPointer<GeographicTransformationFilter>::New() }
{
    m_coordinates->SetNumberOfComponents(3);
    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetData(m_coordinates);
    m_dataSet->SetPoints(points);
    m_spec->SetInputData(m_dataSet);
    m_transform->SetInputConnection(m_spec->GetOutputPort());
    m_transform->OperateInPlaceOn();
}

GeographicTransformationUtil::~GeographicTransformationUtil() = default;

void GeographicTransformationUtil::setSourceSystem(
    const ReferencedCoordinateSystemSpecification & sourceSpec)
{
    m_spec->SetCoordinateSystemSpec(sourceSpec);
}

const ReferencedCoordinateSystemSpecification & GeographicTransformationUtil::sourceSystem() const
{
    return m_spec->GetCoordinateSystemSpec();
}

void GeographicTransformationUtil::setTargetSystem(
    const CoordinateSystemSpecification & targetSpec)
{
    m_transform->SetTargetCoordinateSystem(targetSpec);
}

const CoordinateSystemSpecification & GeographicTransformationUtil::targetSystem() const
{
    return m_transform->GetTargetCoordinateSystem();
}

bool GeographicTransformationUtil::isTransformationSupported() const
{
    return GeographicTransformationFilter::IsTransformationSupported(
        sourceSystem(), targetSystem());

}

bool GeographicTransformationUtil::transformPoints(std::vector<vtkVector3d> & points)
{
    return GeographicTransformationUtil::transformPoints(points.data(), points.size());
}

bool GeographicTransformationUtil::transformPoints(vtkVector3d * points, const size_t numPoints)
{
    if (!isTransformationSupported())
    {
        return false;
    }

    if (numPoints == 0)
    {
        return true;
    }

    if (numPoints > static_cast<size_t>(std::numeric_limits<vtkIdType>::max()) / 3u)
    {
        vtkGenericWarningMacro(
            << "transformPoints: too many input points (exceeding maximum of vtkIdType)");
        return false;
    }

    if (!points)
    {
        return false;
    }

    struct UnRefPointsRawData
    {
        explicit UnRefPointsRawData(GeographicTransformationUtil & util)
            : util{ util } { }
        ~UnRefPointsRawData() { util.releasePoints(); }
        GeographicTransformationUtil & util;
    } unrefPointsRawDataOnReturn(*this);

    auto pointsData = vtkSmartPointer<vtkDoubleArray>::New();
    setPoints(points, numPoints);
    if (!m_transform->GetExecutive()->Update())
    {
        return false;
    }
    auto outputDataSet = vtkPolyData::SafeDownCast(m_transform->GetOutput());
    if (outputDataSet)
    {
        assert(outputDataSet->GetPoints() == m_dataSet->GetPoints());
    }

    return outputDataSet
        && outputDataSet->GetNumberOfPoints() == static_cast<vtkIdType>(numPoints);
}

vtkVector3d GeographicTransformationUtil::transformPoint(
    const vtkVector3d & sourcePoint,
    bool * successPtr)
{
    if (successPtr)
    {
        *successPtr = false;
    }

    vtkVector3d point = sourcePoint;
    const bool success = GeographicTransformationUtil::transformPoints(&point, 1u);

    if (successPtr)
    {
        *successPtr = success;
    }
    return point;
}

void GeographicTransformationUtil::setPoints(vtkVector3d * points, const vtkIdType numPoints)
{
    assert(m_dataSet);
    assert(points);
    if (m_dataSet->GetNumberOfVerts() == numPoints)
    {
        return;
    }
    m_coordinates->SetArray(points[0].GetData(), 3 * static_cast<vtkIdType>(numPoints), 1);
    auto ids = std::vector<vtkIdType>(static_cast<size_t>(numPoints));
    std::iota(ids.begin(), ids.end(), 0);
    auto verts = vtkSmartPointer<vtkCellArray>::New();
    const vtkIdType initId = 0;
    verts->InsertNextCell(1, &initId);
    m_dataSet->SetVerts(verts);
    m_dataSet->Modified();
}

void GeographicTransformationUtil::releasePoints()
{
    m_coordinates->SetArray(nullptr, 0, 0);
}
