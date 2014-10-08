#include "ImageProfilePlot.h"

#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/data_objects/ImageProfileData.h>

using namespace reflectionzeug;


ImageProfilePlot::ImageProfilePlot(ImageProfileData * dataObject)
    : RenderedData(dataObject)
{
}

PropertyGroup * ImageProfilePlot::createConfigGroup()
{
    return new PropertyGroup();
}

vtkProperty * ImageProfilePlot::createDefaultRenderProperty() const
{
    vtkProperty * property = vtkProperty::New();
    property->LightingOff();
    property->SetColor(0, 0, 0);
    return property;
}

vtkActor * ImageProfilePlot::createActor()
{
    VTK_CREATE(vtkPolyDataMapper, mapper);
    mapper->SetInputConnection(dataObject()->processedOutputPort());
    vtkActor * actor = vtkActor::New();
    actor->SetMapper(mapper);
    return actor;
}
