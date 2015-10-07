#include <core/filters/ImagePlaneWidget.h>

#include <cassert>

#include <vtkActor.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataMapper.h>


vtkStandardNewMacro(ImagePlaneWidget);


vtkActor * ImagePlaneWidget::GetTexturePlaneActor()
{
    assert(this->TexturePlaneActor);

    return this->TexturePlaneActor;
}

vtkPolyDataMapper * ImagePlaneWidget::GetTexturePlaneMapper()
{
    auto mapper = GetTexturePlaneActor()->GetMapper();

    assert(dynamic_cast<vtkPolyDataMapper *>(mapper));

    return static_cast<vtkPolyDataMapper *>(mapper);
}
