#include "ImageDataObject.h"

#include <cassert>

#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkEventQtSlotConnect.h>

#include <core/rendered_data/RenderedImageData.h>
#include <core/table_model/QVtkTableModelImage.h>


ImageDataObject::ImageDataObject(const QString & name, vtkImageData * dataSet)
    : DataObject(name, dataSet)
{
    vtkDataArray * data = dataSet->GetPointData()->GetScalars();
    if (data)
    {
        connectObserver("dataChanged", *data, vtkCommand::ModifiedEvent, *this, &ImageDataObject::_dataChanged);
    }
}

bool ImageDataObject::is3D() const
{
    return false;
}

RenderedData * ImageDataObject::createRendered()
{
    return new RenderedImageData(this);
}

void ImageDataObject::addDataArray(vtkDataArray * dataArray)
{
    dataSet()->GetPointData()->AddArray(dataArray);
}

const QString & ImageDataObject::dataTypeName() const
{
    return dataTypeName_s();
}

const QString & ImageDataObject::dataTypeName_s()
{
    static const QString name{ "regular 2D grid" };
    return name;
}


vtkImageData * ImageDataObject::imageData()
{
    return static_cast<vtkImageData *>(dataSet());
}

const vtkImageData * ImageDataObject::imageData() const
{
    return static_cast<const vtkImageData *>(dataSet());
}

const int * ImageDataObject::dimensions()
{
    return imageData()->GetDimensions();
}

const int * ImageDataObject::extent()
{
    return imageData()->GetExtent();
}

const double * ImageDataObject::minMaxValue()
{
    return imageData()->GetScalarRange();
}

QVtkTableModel * ImageDataObject::createTableModel()
{
    QVtkTableModelImage * model = new QVtkTableModelImage();
    model->setDataObject(this);

    return model;
}
