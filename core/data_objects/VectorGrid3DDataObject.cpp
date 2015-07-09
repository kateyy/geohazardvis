#include "VectorGrid3DDataObject.h"

#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

#include <core/rendered_data/RenderedVectorGrid3D.h>
#include <core/table_model/QVtkTableModelVectorGrid3D.h>


VectorGrid3DDataObject::VectorGrid3DDataObject(const QString & name, vtkImageData & dataSet)
    : DataObject(name, &dataSet)
{
    vtkDataArray * data = dataSet.GetPointData()->GetScalars();
    if (!data)
    {
        data = dataSet.GetPointData()->GetVectors();
        dataSet.GetPointData()->SetScalars(data);
    }
}

bool VectorGrid3DDataObject::is3D() const
{
    return true;
}

std::unique_ptr<RenderedData> VectorGrid3DDataObject::createRendered()
{
    return std::make_unique<RenderedVectorGrid3D>(*this);
}

void VectorGrid3DDataObject::addDataArray(vtkDataArray & dataArray)
{
    dataSet()->GetPointData()->AddArray(&dataArray);
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
