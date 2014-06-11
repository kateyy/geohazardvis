#include "RenderedImageData.h"

#include <cassert>

#include <vtkProperty.h>

#include "ImageDataObject.h"
#include "core/Input.h"


RenderedImageData::RenderedImageData(ImageDataObject * dataObject)
    : RenderedData(dataObject)
{
}

const ImageDataObject * RenderedImageData::imageDataObject() const
{
    assert(dynamic_cast<const ImageDataObject*>(dataObject()));
    return static_cast<const ImageDataObject*>(dataObject());
}

vtkProperty * RenderedImageData::createDefaultRenderProperty() const
{
    return vtkProperty::New();
}

vtkActor * RenderedImageData::createActor() const
{
    return imageDataObject()->gridDataInput()->createTexturedPolygonActor();
}
