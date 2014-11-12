#pragma once

#include <core/data_objects/RenderedData3D.h>


class ImageProfileData;


class ImageProfilePlot : public RenderedData3D
{
public:
    ImageProfilePlot(ImageProfileData * dataObject);

    reflectionzeug::PropertyGroup * createConfigGroup() override;

protected:
    vtkProperty * createDefaultRenderProperty() const override;

    vtkSmartPointer<vtkActorCollection> fetchActors() override;

    vtkActor * plotActor();

    void visibilityChangedEvent(bool visible) override;

private:
    vtkSmartPointer<vtkActor> m_plotActor;
};
