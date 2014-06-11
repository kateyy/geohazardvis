#pragma once

#include <core/data_objects/RenderedData.h>
#include <core/core_api.h>


class ImageDataObject;


class CORE_API RenderedImageData : public RenderedData
{
public:
    RenderedImageData(ImageDataObject * dataObject);

    const ImageDataObject * imageDataObject() const;

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkActor * createActor() const override;
};
