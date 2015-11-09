#pragma once

#include <core/canvas_export/CanvasExporter.h>


class vtkImageWriter;
class vtkImageShiftScale;
class vtkWindowToImageFilter;


/** Canvas Exporter base class for some image types. */
class CanvasExporterImages : public CanvasExporter
{
public:
    /** @param writer Subclasses must provide a vtkImageWriter subclass */
    explicit CanvasExporterImages(vtkImageWriter * writer);

    bool write() override;

protected:
    /** provides buffer type and magnification property */
    reflectionzeug::PropertyGroup * createPropertyGroup() override;

    /** complete file name with correct file extension */
    QString verifiedFileName() const;

protected:
    vtkSmartPointer<vtkWindowToImageFilter> m_toImageFilter;
    vtkSmartPointer<vtkImageShiftScale> m_depthToUChar;
    vtkSmartPointer<vtkImageWriter> m_writer;
};
