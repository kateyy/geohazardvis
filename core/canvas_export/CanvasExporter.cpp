/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
