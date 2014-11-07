#pragma once

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

    reflectionzeug::PropertyGroup * propertyGroup();

    void setRenderWindow(vtkRenderWindow * renderWindow);
    vtkRenderWindow * renderWindow();

    const QString & outputFormat() const;
    const QString & outputFileName() const;
    void setOutputFileName(const QString & fileName);

    virtual bool write() = 0;

    template<typename T> 
    static CanvasExporter * newInstance();

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
CanvasExporter * CanvasExporter::newInstance()
{
    return new T();
}
