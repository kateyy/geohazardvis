#include "VectorGrid3DDataObject.h"

#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

#include <core/rendered_data/RenderedVectorGrid3D.h>
#include <core/table_model/QVtkTableModelVectorGrid3D.h>


VectorGrid3DDataObject::VectorGrid3DDataObject(QString name, vtkImageData * dataSet)
    : DataObject(name, dataSet)
{
    vtkDataArray * data = dataSet->GetPointData()->GetScalars();
    if (!data)
    {
        data = dataSet->GetPointData()->GetVectors();
        dataSet->GetPointData()->SetScalars(data);
    }

    if (data->GetNumberOfComponents() == 6)
    {
        vtkSmartPointer<vtkDataArray> vectors1 = data->NewInstance();
        vtkSmartPointer<vtkDataArray> vectors2 = data->NewInstance();
        vectors1->SetNumberOfComponents(3);
        vectors2->SetNumberOfComponents(3);
        vectors1->SetName((QString::fromUtf8(data->GetName()) + " (1)").toUtf8().data());
        vectors2->SetName((QString::fromUtf8(data->GetName()) + " (2)").toUtf8().data());
        vtkIdType numTuples = data->GetNumberOfTuples();
        vectors1->SetNumberOfTuples(numTuples);
        vectors2->SetNumberOfTuples(numTuples);

        for (vtkIdType i = 0; i < numTuples; ++i)
        {
            vectors1->SetTuple(i, data->GetTuple(i));
            vectors2->SetTuple(i, &data->GetTuple(i)[3]);
        }
        dataSet->GetPointData()->AddArray(vectors1);
        dataSet->GetPointData()->AddArray(vectors2);
    }
}

VectorGrid3DDataObject::~VectorGrid3DDataObject() = default;

bool VectorGrid3DDataObject::is3D() const
{
    return true;
}

RenderedData * VectorGrid3DDataObject::createRendered()
{
    return new RenderedVectorGrid3D(this);
}

void VectorGrid3DDataObject::addDataArray(vtkDataArray * dataArray)
{
    dataSet()->GetPointData()->AddArray(dataArray);
}

const QString & VectorGrid3DDataObject::dataTypeName() const
{
    return dataTypeName_s();
}

const QString & VectorGrid3DDataObject::dataTypeName_s()
{
    static const QString name{ "3D vector grid" };
    return name;
}

const int * VectorGrid3DDataObject::dimensions()
{
    return static_cast<vtkImageData *>(dataSet())->GetDimensions();
}

const int * VectorGrid3DDataObject::extent()
{
    return static_cast<vtkImageData *>(dataSet())->GetExtent();
}

int VectorGrid3DDataObject::numberOfComponents()
{
    return dataSet()->GetPointData()->GetScalars()->GetNumberOfComponents();
}

const double * VectorGrid3DDataObject::scalarRange(int component)
{
    return dataSet()->GetPointData()->GetScalars()->GetRange(component);
}

QVtkTableModel * VectorGrid3DDataObject::createTableModel()
{
    QVtkTableModel * model = new QVtkTableModelVectorGrid3D();
    model->setDataObject(this);

    return model;
}
