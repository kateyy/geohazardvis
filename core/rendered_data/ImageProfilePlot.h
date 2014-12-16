#pragma once

#include <core/rendered_data/RenderedData3D.h>


class ImageProfileData;


class ImageProfilePlot : public RenderedData3D
{
public:
    ImageProfilePlot(ImageProfileData * dataObject);

    reflectionzeug::PropertyGroup * createConfigGroup() override;

protected:
    vtkProperty * createDefaultRenderProperty() const override;

    vtkSmartPointer<vtkProp3DCollection> fetchViewProps3D() override;

    vtkActor * plotActor();

    void visibilityChangedEvent(bool visible) override;

private:
    vtkSmartPointer<vtkActor> m_plotActor;
};
