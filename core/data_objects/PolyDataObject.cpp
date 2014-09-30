#include "PolyDataObject.h"

#include <cassert>

#include <vtkFloatArray.h>
#include <vtkIdTypeArray.h>

#include <vtkPolyDataNormals.h>
#include <vtkCellCenters.h>

#include <vtkPolyData.h>
#include <vtkCellData.h>
#include <vtkCellIterator.h>

#include <vtkPolygon.h>

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

    if (!dataSet->GetCellData()->HasArray("centroid"))
    {
        vtkSmartPointer<vtkFloatArray> centroids = vtkSmartPointer<vtkFloatArray>::New();
        centroids->SetName("centroid");
        centroids->SetNumberOfComponents(3);
        centroids->SetNumberOfTuples(dataSet->GetNumberOfCells());
        vtkSmartPointer<vtkCellIterator> it = vtkSmartPointer<vtkCellIterator>::Take(dataSet->NewCellIterator());
        double centroid[3];
        vtkSmartPointer<vtkIdTypeArray> idArray = vtkSmartPointer<vtkIdTypeArray>::New();
        vtkPoints * polyDataPoints = dataSet->GetPoints();
        vtkIdType centroidIndex = 0;
        for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
        {
            idArray->SetArray(it->GetPointIds()->GetPointer(0), it->GetNumberOfPoints(), true);
            vtkPolygon::ComputeCentroid(
                idArray,
                polyDataPoints,
                centroid);
            centroids->SetTuple(centroidIndex++, centroid);
        }
        dataSet->GetCellData()->AddArray(centroids);
    }

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
    m_cellCenters->Update();
    return m_cellCenters->GetOutput();
}

vtkAlgorithmOutput * PolyDataObject::processedOutputPort()
{
    return m_cellCenters->GetOutputPort();
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