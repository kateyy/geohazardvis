#pragma once

#include "config.h"
#if VTK_module_IOExport // this module is currently not supported with VTK OpenGL2 rendering back-end 

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
