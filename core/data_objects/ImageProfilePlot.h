#pragma once

#include <core/data_objects/RenderedData.h>


class ImageProfileData;


class ImageProfilePlot : public RenderedData
{
public:
    ImageProfilePlot(ImageProfileData * dataObject);

    reflectionzeug::PropertyGroup * createConfigGroup() override;

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkActor * createActor() override;
};