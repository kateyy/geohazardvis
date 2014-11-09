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

    /** complete file name with correct file extension */
    QString verifiedFileName();

protected:
    vtkSmartPointer<vtkWindowToImageFilter> m_toImageFilter;
};
