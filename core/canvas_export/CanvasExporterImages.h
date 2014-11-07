#pragma once

#include <core/canvas_export/CanvasExporter.h>

class vtkWindowToImageFilter;


/** Canvas Exporter base class for some image types. */
class CanvasExporterImages : public CanvasExporter
{
public:
    CanvasExporterImages();

protected:
    /** provides buffer type and magnification property */
    reflectionzeug::PropertyGroup * createPropertyGroup() override;

protected:
    vtkSmartPointer<vtkWindowToImageFilter> m_toImageFilter;
};
