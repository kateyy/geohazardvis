#pragma once

#include <core/rendered_data/RenderedData3D.h>


class vtkTexture;

class ImageDataObject;


class CORE_API RenderedImageData : public RenderedData3D
{
public:
    RenderedImageData(ImageDataObject * dataObject);

    const ImageDataObject * imageDataObject() const;

    reflectionzeug::PropertyGroup * createConfigGroup() override;

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkSmartPointer<vtkActorCollection> fetchActors() override;
    vtkActor * imageActor();

    void scalarsForColorMappingChangedEvent() override;
    void colorMappingGradientChangedEvent() override;
    void visibilityChangedEvent(bool visible) override;

private:
    vtkSmartPointer<vtkTexture> m_texture;
    vtkSmartPointer<vtkActor> m_imageActor;
};
