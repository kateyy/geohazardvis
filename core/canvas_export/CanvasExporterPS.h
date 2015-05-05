#pragma once

#include "config.h"
#if VTK_has_GLExport2PS

#include <core/canvas_export/CanvasExporter.h>


class vtkGL2PSExporter;


class CanvasExporterPS : public CanvasExporter
{
public:
    CanvasExporterPS();

    QString fileExtension() const override;

    bool write() override;

protected:
    reflectionzeug::PropertyGroup * createPropertyGroup() override;

    QStringList fileFormats() const override;

private:
    vtkSmartPointer<vtkGL2PSExporter> m_exporter;
};

#endif
