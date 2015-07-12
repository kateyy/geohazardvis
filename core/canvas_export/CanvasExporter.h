#pragma once

#include <memory>

#include <QString>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class QStringList;
class vtkRenderWindow;
namespace reflectionzeug { class PropertyGroup; }


/** Abstract base class for canvas (render window contents) exporters. */
class CORE_API CanvasExporter
{
public:
    CanvasExporter();
    virtual ~CanvasExporter();

    /** @return file extension of the configured file format */
    virtual QString fileExtension() const;
    /** @return friendly name of the configured file format */
    const QString & outputFormat() const;

    reflectionzeug::PropertyGroup * propertyGroup();

    void setRenderWindow(vtkRenderWindow * renderWindow);
    vtkRenderWindow * renderWindow();

    const QString & outputFileName() const;
    void setOutputFileName(const QString & fileName);

    virtual bool write() = 0;

    template<typename T> 
    static std::unique_ptr<CanvasExporter> newInstance();

protected:
    virtual reflectionzeug::PropertyGroup * createPropertyGroup() = 0;

    friend class CanvasExporterRegistry;
    virtual QStringList fileFormats() const = 0;
    void setOutputFormat(const QString & format);

private:
    vtkSmartPointer<vtkRenderWindow> m_renderWindow;

    QString m_format;
    QString m_fileName;

    reflectionzeug::PropertyGroup * m_propertyGroup;
};

template<typename T>
std::unique_ptr<CanvasExporter> CanvasExporter::newInstance()
{
    return std::make_unique<T>();
}
