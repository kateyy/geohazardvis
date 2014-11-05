#include "ImageDataObject.h"

#include <cassert>

#include <vtkImageData.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkEventQtSlotConnect.h>

#include <core/data_objects/RenderedImageData.h>
#include <core/table_model/QVtkTableModelImage.h>


namespace
{
    const QString s_dataTypeName = "regular 2D grid";
}

ImageDataObject::ImageDataObject(QString name, vtkImageData * dataSet)
    : DataObject(name, dataSet)
{
    vtkDataArray * data = dataSet->GetCellData()->GetScalars();
    vtkQtConnect()->Connect(data, vtkCommand::ModifiedEvent, this, SLOT(_dataChanged()));
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
    dataSet()->GetCellData()->AddArray(dataArray);

    emit attributeArraysChanged();
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
