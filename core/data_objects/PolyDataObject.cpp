#include "PolyDataObject.h"

#include <cassert>

#include <vtkPolyDataNormals.h>
#include <vtkCellCenters.h>

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

QVtkTableModel * PolyDataObject::createTableModel()
{
    QVtkTableModel * model = new QVtkTableModelPolyData();
    model->setDataObject(this);

    return model;
}