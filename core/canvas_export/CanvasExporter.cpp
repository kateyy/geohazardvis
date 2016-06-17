#include "CanvasExporter.h"

#include <vtkRenderWindow.h>

#include <reflectionzeug/PropertyGroup.h>


CanvasExporter::CanvasExporter()
{
}

CanvasExporter::~CanvasExporter() = default;

QString CanvasExporter::fileExtension() const
{
    return outputFormat().toLower();
}

const QString & CanvasExporter::outputFormat() const
{
    return m_format;
}

reflectionzeug::PropertyGroup * CanvasExporter::propertyGroup()
{
    if (!m_propertyGroup)
    {
        m_propertyGroup = createPropertyGroup();
    }

    return m_propertyGroup.get();
}

void CanvasExporter::setRenderWindow(vtkRenderWindow * renderWindow)
{
    m_renderWindow = renderWindow;
}

vtkRenderWindow * CanvasExporter::renderWindow()
{
    return m_renderWindow;
}

const QString & CanvasExporter::outputFileName() const
{
    return m_fileName;
}

void CanvasExporter::setOutputFileName(const QString & fileName)
{
    m_fileName = fileName;
}

bool CanvasExporter::openGLContextSupported()
{
    return (m_renderWindow != nullptr);
}

void CanvasExporter::setOutputFormat(const QString & format)
{
    m_format = format;
}
