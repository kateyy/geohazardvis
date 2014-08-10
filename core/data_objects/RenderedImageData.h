#pragma once

#include <core/data_objects/RenderedData.h>
#include <core/core_api.h>


class vtkPolyDataMapper;
class vtkTexture;

class ImageDataObject;


class CORE_API RenderedImageData : public RenderedData
{
public:
    RenderedImageData(ImageDataObject * dataObject);

    const ImageDataObject * imageDataObject() const;

    reflectionzeug::PropertyGroup * createConfigGroup() override;

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkActor * createActor() override;

    void updateScalarToColorMapping() override;

private:
    vtkPolyDataMapper * createMapper() const;

    vtkTexture * buildTexture() const;

};
