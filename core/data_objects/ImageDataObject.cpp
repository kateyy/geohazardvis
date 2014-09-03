#include "ImageDataObject.h"

#include <cassert>

#include <vtkImageData.h>

#include <core/data_objects/RenderedImageData.h>


namespace
{
    QString s_dataTypeName = "regular 2D grid";
}

ImageDataObject::ImageDataObject(QString name, vtkImageData * dataSet)
    : DataObject(name, dataSet)
{
}

RenderedData * ImageDataObject::createRendered()
{
    return new RenderedImageData(this);
}

QString ImageDataObject::dataTypeName() const
{
    return s_dataTypeName;
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

const double * ImageDataObject::minMaxValue()
{
    return imageData()->GetScalarRange();
}
