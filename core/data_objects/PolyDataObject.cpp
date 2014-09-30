#include "PolyDataObject.h"

#include <cassert>
#include <cmath>

#include <vtkPolyDataNormals.h>
#include <vtkCellCenters.h>

#include <vtkCellData.h>
#include <vtkCell.h>

#include <vtkMath.h>
#include <vtkTransform.h>

#include <core/vtkhelper.h>
#include <core/data_objects/RenderedPolyData.h>
#include <core/table_model/QVtkTableModelPolyData.h>


namespace
{
    const QString s_dataTypeName = "polygonal mesh";
}

PolyDataObject::PolyDataObject(QString name, vtkPolyData * dataSet)
    : DataObject(name, dataSet)
    , m_cellNormals(vtkSmartPointer<vtkPolyDataNormals>::New())
    , m_cellCenters(vtkSmartPointer<vtkCellCenters>::New())
{
    m_cellNormals->ComputeCellNormalsOn();
    m_cellNormals->ComputePointNormalsOff();
    m_cellNormals->SetInputData(dataSet);

    m_cellCenters->SetInputConnection(m_cellNormals->GetOutputPort());
}

bool PolyDataObject::is3D() const
{
    return true;
}

vtkDataSet * PolyDataObject::processedDataSet()
{
    m_cellNormals->Update();
    return m_cellNormals->GetOutput();
}

vtkAlgorithmOutput * PolyDataObject::processedOutputPort()
{
    return m_cellNormals->GetOutputPort();
}

vtkPolyData * PolyDataObject::cellCenters()
{
    m_cellCenters->Update();
    return m_cellCenters->GetOutput();
}

vtkAlgorithmOutput * PolyDataObject::cellCentersOutputPort()
{
    return m_cellCenters->GetOutputPort();
}


RenderedData * PolyDataObject::createRendered()
{
    return new RenderedPolyData(this);
}

QString PolyDataObject::dataTypeName() const
{
    return s_dataTypeName;
}

bool PolyDataObject::setCellCenterComponent(vtkIdType cellId, int component, double value)
{
    assert(component >= 0 && component < 3);
    assert(cellId <= dataSet()->GetNumberOfCells());
    vtkCell * cell = dataSet()->GetCell(cellId);
    vtkIdList * pointIds = cell->GetPointIds();

    vtkPoints * centers = cellCenters()->GetPoints();
    vtkPoints * vertices = static_cast<vtkPolyData *>(dataSet())->GetPoints();

    double oldValue = centers->GetPoint(cellId)[component];
    double valueDelta = value - oldValue;

    // apply the value delta to all vertices of the triangle
    double point[3];
    for (int i = 0; i < pointIds->GetNumberOfIds(); ++i)
    {
        vtkIdType pointId = pointIds->GetId(i);
        vertices->GetPoint(pointId, point);
        point[component] += valueDelta;
        vertices->SetPoint(pointId, point);
    }

    dataSet()->Modified();

    return true;
}

bool PolyDataObject::setCellNormalComponent(vtkIdType cellId, int component, double value)
{
    assert(component >= 0 && component < 3);
    assert(cellId <= dataSet()->GetNumberOfCells());

    vtkDataArray * normals = processedDataSet()->GetCellData()->GetNormals();
    assert(normals);
    double oldNormal[3], newNormal[3];
    normals->GetTuple(cellId, oldNormal);
    normals->GetTuple(cellId, newNormal);

    newNormal[component] = value;
    vtkMath::Normalize(newNormal);

    double angleRad = std::acos(vtkMath::Dot(oldNormal, newNormal));
    // angle is too small, so don't apply the rotation
    if (std::abs(angleRad) < 0.000001)
         return false;

    // we have to flip the polygon. Find any suitable axis.
    if (std::abs(angleRad) - vtkMath::Pi() < 0.0001)
    {
        newNormal[0] += 1.;
    }

    double rotationAxis[3];
    vtkMath::Cross(oldNormal, newNormal, rotationAxis);

    // use rotation axis, apply it at the polygon center
    double center[3];
    cellCenters()->GetPoint(cellId, center);
    VTK_CREATE(vtkTransform, rotation);
    rotation->Translate(center);
    rotation->RotateWXYZ(vtkMath::DegreesFromRadians(angleRad), rotationAxis);
    rotation->Translate(-center[0], -center[1], -center[2]);


    // apply the rotation to all vertices of the triangle
    vtkCell * cell = dataSet()->GetCell(cellId);
    vtkIdList * pointIds = cell->GetPointIds();
    vtkPoints * vertices = static_cast<vtkPolyData *>(dataSet())->GetPoints();
    double point[3];
    for (int i = 0; i < pointIds->GetNumberOfIds(); ++i)
    {
        vtkIdType pointId = pointIds->GetId(i);
        vertices->GetPoint(pointId, point);
        rotation->TransformPoint(point, point);
        vertices->SetPoint(pointId, point);
    }

    dataSet()->Modified();

    return true;
}

QVtkTableModel * PolyDataObject::createTableModel()
{
    QVtkTableModel * model = new QVtkTableModelPolyData();
    model->setDataObject(this);

    return model;
}