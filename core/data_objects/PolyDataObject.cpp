#include "PolyDataObject.h"

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include <vtkCell.h>
#include <vtkCellCenters.h>
#include <vtkCellData.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPolyDataNormals.h>
#include <vtkTransform.h>

#include <core/rendered_data/RenderedPolyData.h>
#include <core/table_model/QVtkTableModelPolyData.h>


PolyDataObject::PolyDataObject(const QString & name, vtkPolyData & dataSet)
    : GenericPolyDataObject(name, dataSet)
    , m_cellNormals{ vtkSmartPointer<vtkPolyDataNormals>::New() }
    , m_cellCenters{}
    , m_is2p5D{ Is2p5D::unchecked }
{
    m_cellNormals->ComputeCellNormalsOn();
    m_cellNormals->ComputePointNormalsOff();
}

PolyDataObject::~PolyDataObject() = default;

vtkAlgorithmOutput * PolyDataObject::processedOutputPort()
{
    m_cellNormals->SetInputConnection(CoordinateTransformableDataObject::processedOutputPort());
    return m_cellNormals->GetOutputPort();
}

vtkPolyData * PolyDataObject::cellCenters()
{
    setupCellCenters();

    m_cellCenters->Update();
    return m_cellCenters->GetOutput();
}

vtkAlgorithmOutput * PolyDataObject::cellCentersOutputPort()
{
    setupCellCenters();

    return m_cellCenters->GetOutputPort();
}

std::unique_ptr<RenderedData> PolyDataObject::createRendered()
{
    return std::make_unique<RenderedPolyData>(*this);
}

void PolyDataObject::addDataArray(vtkDataArray & dataArray)
{
    dataSet()->GetCellData()->AddArray(&dataArray);
}

const QString & PolyDataObject::dataTypeName() const
{
    return dataTypeName_s();
}

const QString & PolyDataObject::dataTypeName_s()
{
    static const QString name{ "Polygonal Mesh" };

    return name;
}

bool PolyDataObject::is2p5D()
{
    if (m_is2p5D != Is2p5D::unchecked)
    {
        return m_is2p5D == Is2p5D::yes;
    }

    m_is2p5D = Is2p5D::yes;

    auto dsWithNormals = processedDataSet();
    auto normals = dsWithNormals->GetCellData()->GetNormals();
    if (!normals)   // point data?
    {
        normals = dsWithNormals->GetPointData()->GetNormals();
    }
    if (!normals)   // cannot be determined, assume "no"
    {
        m_is2p5D = Is2p5D::no;
        return false;
    }
    
    for (vtkIdType i = 0; i < normals->GetNumberOfTuples(); ++i)
    {
        std::array<double, 3> normal;
        normals->GetTuple(i, normal.data());
        if (normal[2] < std::numeric_limits<double>::epsilon())
        {
            m_is2p5D = Is2p5D::no;
            break;
        }
    }

    return m_is2p5D == Is2p5D::yes;
}

double PolyDataObject::cellCenterComponent(vtkIdType cellId, int component, bool * validIdPtr)
{
    auto & centroids = *cellCenters()->GetPoints()->GetData();

    const auto isValid = cellId < centroids.GetNumberOfTuples()
        && component < centroids.GetNumberOfComponents();

    if (validIdPtr)
    {
        *validIdPtr = isValid;
    }

    if (!isValid)
    {
        return{};
    }

    return centroids.GetComponent(cellId, component);
}

bool PolyDataObject::setCellCenterComponent(vtkIdType cellId, int component, double value)
{
    auto & ds = polyDataSet();
    assert(component >= 0 && component < 3);
    assert(cellId <= ds.GetNumberOfCells());
    auto cell = ds.GetCell(cellId);
    auto pointIds = cell->GetPointIds();

    auto centers = cellCenters()->GetPoints();
    assert(centers);
    auto vertices = ds.GetPoints();

    const double oldValue = [centers, cellId, component] () -> double {
        std::array<double, 3> point;
        centers->GetPoint(cellId, point.data());
        return point[component];
    }();
    const double valueDelta = value - oldValue;

    // apply the value delta to all vertices of the triangle
    for (int i = 0; i < pointIds->GetNumberOfIds(); ++i)
    {
        std::array<double, 3> point;
        const auto pointId = pointIds->GetId(i);
        vertices->GetPoint(pointId, point.data());
        point[component] += valueDelta;
        vertices->SetPoint(pointId, point.data());
    }

    ds.Modified();

    return true;
}

double PolyDataObject::cellNormalComponent(vtkIdType cellId, int component, bool * validIdPtr)
{
    auto normals = processedDataSet()->GetCellData()->GetNormals();

    const auto isValid = normals
        && cellId < normals->GetNumberOfTuples()
        && component < normals->GetNumberOfComponents();

    if (validIdPtr)
    {
        *validIdPtr = isValid;
    }

    if (!isValid)
    {
        return{};
    }

    return normals->GetComponent(cellId, component);
}

bool PolyDataObject::setCellNormalComponent(vtkIdType cellId, int component, double value)
{
    auto & ds = polyDataSet();
    assert(component >= 0 && component < 3);
    assert(cellId <= ds.GetNumberOfCells());

    auto normals = processedDataSet()->GetCellData()->GetNormals();

    const auto isValid = normals
        && cellId < normals->GetNumberOfTuples()
        && component < normals->GetNumberOfComponents();

    if (!isValid)
    {
        return false;
    }

    std::array<double, 3> oldNormal, newNormal;
    normals->GetTuple(cellId, oldNormal.data());
    newNormal = oldNormal;

    newNormal[component] = value;
    vtkMath::Normalize(newNormal.data());

    const double angleRad = std::acos(vtkMath::Dot(oldNormal.data(), newNormal.data()));
    // angle is too small, so don't apply the rotation
    if (std::abs(angleRad) < 0.000001)
    {
        return false;
    }

    // we have to flip the polygon. Find any suitable axis.
    if (std::abs(angleRad) - vtkMath::Pi() < 0.0001)
    {
        newNormal[0] += 1.;
    }

    std::array<double, 3> rotationAxis;
    vtkMath::Cross(oldNormal.data(), newNormal.data(), rotationAxis.data());

    // use rotation axis, apply it at the polygon center
    std::array<double, 3> center;
    cellCenters()->GetPoint(cellId, center.data());
    auto rotation = vtkSmartPointer<vtkTransform>::New();
    rotation->Translate(center.data());
    rotation->RotateWXYZ(vtkMath::DegreesFromRadians(angleRad), rotationAxis.data());
    rotation->Translate(-center[0], -center[1], -center[2]);


    // apply the rotation to all vertices of the triangle
    auto cell = ds.GetCell(cellId);
    auto pointIds = cell->GetPointIds();
    auto vertices = ds.GetPoints();
    std::array<double, 3> point;
    for (int i = 0; i < pointIds->GetNumberOfIds(); ++i)
    {
        const vtkIdType pointId = pointIds->GetId(i);
        vertices->GetPoint(pointId, point.data());
        rotation->TransformPoint(point.data(), point.data());
        vertices->SetPoint(pointId, point.data());
    }

    ds.Modified();

    return true;
}

std::unique_ptr<QVtkTableModel> PolyDataObject::createTableModel()
{
    std::unique_ptr<QVtkTableModel> model = std::make_unique<QVtkTableModelPolyData>();
    model->setDataObject(this);

    return model;
}

void PolyDataObject::setupCellCenters()
{
    if (m_cellCenters)
    {
        return;
    }

    m_cellCenters = vtkSmartPointer<vtkCellCenters>::New();
    m_cellCenters->SetInputConnection(processedOutputPort());
}
