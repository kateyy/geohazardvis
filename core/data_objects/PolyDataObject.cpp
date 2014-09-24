#include "PolyDataObject.h"

#include <cassert>

#include <vtkFloatArray.h>
#include <vtkIdTypeArray.h>

#include <vtkPolyDataNormals.h>

#include <vtkPolyData.h>
#include <vtkCellData.h>
#include <vtkCellIterator.h>

#include <vtkPolygon.h>

#include <core/vtkhelper.h>
#include <core/data_objects/RenderedPolyData.h>


namespace
{
    const QString s_dataTypeName = "polygonal mesh";
}

PolyDataObject::PolyDataObject(QString name, vtkPolyData * dataSet)
    : DataObject(name, dataSet)
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

    if (!dataSet->GetCellData()->HasArray("Normals"))
    {
        VTK_CREATE(vtkPolyDataNormals, inputNormals);
        inputNormals->ComputePointNormalsOff();
        inputNormals->ComputeCellNormalsOn();
        inputNormals->SetInputDataObject(dataSet);
        inputNormals->Update();

        dataSet->GetCellData()->SetNormals(inputNormals->GetOutput()->GetCellData()->GetNormals());
    }
}

bool PolyDataObject::is3D() const
{
    return true;
}

RenderedData * PolyDataObject::createRendered()
{
    return new RenderedPolyData(this);
}

QString PolyDataObject::dataTypeName() const
{
    return s_dataTypeName;
}
