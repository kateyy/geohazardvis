#pragma once

#include <vtkImagePlaneWidget.h>

#include <core/core_api.h>


class vtkPolyDataMapper;


class CORE_API ImagePlaneWidget : public vtkImagePlaneWidget
{
public:
    vtkTypeMacro(ImagePlaneWidget, vtkImagePlaneWidget);
    static ImagePlaneWidget * New();

    vtkActor * GetTexturePlaneActor();
    vtkPolyDataMapper * GetTexturePlaneMapper();

protected:
    ImagePlaneWidget() = default;

private:
    ImagePlaneWidget(const ImagePlaneWidget &) = delete;
    void operator=(const ImagePlaneWidget &) = delete;
};
