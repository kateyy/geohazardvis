#pragma once

#include <core/canvas_export/CanvasExporter.h>


class vtkGL2PSExporter;


class CanvasExporterPS : public CanvasExporter
{
public:
    CanvasExporterPS();
    ~CanvasExporterPS() override;

    QString fileExtension() const override;

    bool write() override;

    bool openGLContextSupported() override;

protected:
    std::unique_ptr<reflectionzeug::PropertyGroup> createPropertyGroup() override;

    QStringList fileFormats() const override;

private:
    vtkSmartPointer<vtkGL2PSExporter> m_exporter;

    static const bool s_isRegistered;
};
