#include "PolyDataObject.h"

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include <QDebug>

#include <vtkCell.h>
#include <vtkCellCenters.h>
#include <vtkCellData.h>
#include <vtkExecutive.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPolyDataNormals.h>
#include <vtkTransform.h>

#include <core/types.h>
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

std::unique_ptr<DataObject> PolyDataObject::newInstance(const QString & name, vtkDataSet * dataSet) const
{
    if (auto poly = vtkPolyData::SafeDownCast(dataSet))
    {
        return std::make_unique<PolyDataObject>(name, *poly);
    }
    return{};
}

IndexType PolyDataObject::defaultAttributeLocation() const
{
    return IndexType::cells;
}

vtkPolyData * PolyDataObject::cellCenters()
{
    setupCellCenters();
    if (!m_cellCenters->GetExecutive()->Update())
    {
        return nullptr;
    }
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

    auto dsWithNormals = processedOutputDataSet();
    if (!dsWithNormals)
    {
        qWarning() << "Pipeline failure in" << name();
        return false;
    }

    m_is2p5D = Is2p5D::yes;

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
    if (validIdPtr)
    {
        *validIdPtr = false;
    }

    auto cellCentersChecked = cellCenters();
    if (!cellCentersChecked)
    {
        return;
    }
    auto & centroids = *cellCentersChecked->GetPoints()->GetData();

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

    auto cellCentersChecked = cellCenters();
    if (!cellCentersChecked)
    {
        return false;
    }
    auto centers = cellCentersChecked->GetPoints();
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
    auto dsWithNormals = processedOutputDataSet();
    if (!dsWithNormals)
    {
        qWarning() << "Pipeline failure in" << name();
        return{};
    }
    auto normals = dsWithNormals->GetCellData()->GetNormals();

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

    auto dsWithNormals = processedOutputDataSet();
    if (!dsWithNormals)
    {
        qWarning() << "Pipeline failure in" << name();
        return false;
    }
    auto normals = dsWithNormals->GetCellData()->GetNormals();

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
    auto cellCentersChecked = cellCenters();
    if (!cellCentersChecked)
    {
        return false;
    }
    cellCentersChecked->GetPoint(cellId, center.data());
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

vtkAlgorithmOutput * PolyDataObject::processedOutputPortInternal()
{
    m_cellNormals->SetInputConnection(CoordinateTransformableDataObject::processedOutputPortInternal());
    return m_cellNormals->GetOutputPort();
}

std::unique_ptr<QVtkTableModel> PolyDataObject::createTableModel()
{
    auto model = std::make_unique<QVtkTableModelPolyData>();
    model->setDataObject(this);

    return std::move(model);
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
