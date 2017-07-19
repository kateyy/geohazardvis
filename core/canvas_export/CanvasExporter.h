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

    /** @return whether the current OpenGL context / driver supports all required functions
      * Set a valid render window before calling this function! */
    virtual bool openGLContextSupported();

    template<typename T> 
    static std::unique_ptr<CanvasExporter> newInstance();

protected:
    virtual std::unique_ptr<reflectionzeug::PropertyGroup> createPropertyGroup() = 0;

    friend class CanvasExporterRegistry;
    virtual QStringList fileFormats() const = 0;
    void setOutputFormat(const QString & format);

private:
    vtkSmartPointer<vtkRenderWindow> m_renderWindow;

    QString m_format;
    QString m_fileName;

    std::unique_ptr<reflectionzeug::PropertyGroup> m_propertyGroup;
};

template<typename T>
std::unique_ptr<CanvasExporter> CanvasExporter::newInstance()
{
    return std::make_unique<T>();
}
