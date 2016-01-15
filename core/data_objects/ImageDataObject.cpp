#include "ImageDataObject.h"

#include <cassert>
#include <limits>

#include <QDebug>

#include <vtkCommand.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

#include <core/rendered_data/RenderedImageData.h>
#include <core/table_model/QVtkTableModelImage.h>
#include <core/utility/conversions.h>


ImageDataObject::ImageDataObject(const QString & name, vtkImageData & dataSet)
    : DataObject(name, &dataSet)
    , m_extent()
{
    vtkVector3d spacing;
    bool spacingChanged = false;
    dataSet.GetSpacing(spacing.GetData());
    for (int i = 0; i < 3; ++i)
    {
        if (spacing[i] < std::numeric_limits<double>::epsilon())
        {
            spacing[i] = 1;
            spacingChanged = true;
        }
    }

    if (spacingChanged)
    {
        qDebug() << "Fixing invalid image spacing in " << name << "(" + 
            vector3ToString(vtkVector3d(dataSet.GetSpacing())) + " to " + vector3ToString(spacing) + ")";
        dataSet.SetSpacing(spacing.GetData());
    }

    vtkDataArray * data = dataSet.GetPointData()->GetScalars();
    if (data)
    {
        connectObserver("dataChanged", *data, vtkCommand::ModifiedEvent, *this, &ImageDataObject::_dataChanged);
    }

    dataSet.GetExtent(m_extent.data());
}

ImageDataObject::~ImageDataObject() = default;

bool ImageDataObject::is3D() const
{
    return false;
}

std::unique_ptr<RenderedData> ImageDataObject::createRendered()
{
    return std::make_unique<RenderedImageData>(*this);
}

void ImageDataObject::addDataArray(vtkDataArray & dataArray)
{
    dataSet()->GetPointData()->AddArray(&dataArray);
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

std::unique_ptr<QVtkTableModel> ImageDataObject::createTableModel()
{
    std::unique_ptr<QVtkTableModel> model = std::make_unique<QVtkTableModelImage>();
    model->setDataObject(this);

    return model;
}

bool ImageDataObject::checkIfStructureChanged()
{
    if (DataObject::checkIfStructureChanged())
    {
        return true;
    }

    decltype(m_extent) newExtent;
    imageData()->GetExtent(newExtent.data());

    bool changed = newExtent != m_extent;

    if (changed)
    {
        m_extent = newExtent;
    }

    return changed;
}
