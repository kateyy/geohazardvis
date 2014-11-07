#include "CanvasExporter.h"

#include <vtkRenderWindow.h>

#include <reflectionzeug/PropertyGroup.h>


CanvasExporter::CanvasExporter()
    : m_propertyGroup(nullptr)
{
}

CanvasExporter::~CanvasExporter()
{
    delete m_propertyGroup;
}

reflectionzeug::PropertyGroup * CanvasExporter::propertyGroup()
{
    if (!m_propertyGroup)
        m_propertyGroup = createPropertyGroup();

    return m_propertyGroup;
}

void CanvasExporter::setRenderWindow(vtkRenderWindow * renderWindow)
{
    m_renderWindow = renderWindow;
}

vtkRenderWindow * CanvasExporter::renderWindow()
{
    return m_renderWindow;
}

const QString & CanvasExporter::outputFormat() const
{
    return m_format;
}

const QString & CanvasExporter::outputFileName() const
{
    return m_fileName;
}

void CanvasExporter::setOutputFileName(const QString & fileName)
{
    m_fileName = fileName;
}

void CanvasExporter::setOutputFormat(const QString & format)
{
    m_format = format;
}
