#pragma once

#include <core/data_objects/RenderedData.h>
#include <core/core_api.h>


class vtkPolyDataMapper;

class ImageDataObject;


class CORE_API RenderedImageData : public RenderedData
{
public:
    RenderedImageData(ImageDataObject * dataObject);

    const ImageDataObject * imageDataObject() const;

    reflectionzeug::PropertyGroup * configGroup() override;

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkActor * createActor() const override;

    void updateScalarToColorMapping() override;

private:
    vtkPolyDataMapper * createMapper() const;

};
