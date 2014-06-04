#include "RenderedImageData.h"

#include <cassert>

#include <vtkProperty.h>

#include "ImageDataObject.h"
#include "core/Input.h"


RenderedImageData::RenderedImageData(std::shared_ptr<const ImageDataObject> dataObject)
    : RenderedData(dataObject)
{
}

std::shared_ptr<const ImageDataObject> RenderedImageData::imageDataObject() const
{
    assert(dynamic_cast<const ImageDataObject*>(dataObject().get()));
    return std::static_pointer_cast<const ImageDataObject>(dataObject());
}

vtkProperty * RenderedImageData::createDefaultRenderProperty() const
{
    return vtkProperty::New();
}

vtkActor * RenderedImageData::createActor() const
{
    return imageDataObject()->gridDataInput()->createTexturedPolygonActor();
}
