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
    ~CanvasExporterImages() override;

    bool write() override;

protected:
    /** provides buffer type and magnification property */
    std::unique_ptr<reflectionzeug::PropertyGroup> createPropertyGroup() override;

    /** complete file name with correct file extension */
    QString verifiedFileName() const;

protected:
    vtkSmartPointer<vtkWindowToImageFilter> m_toImageFilter;
    vtkSmartPointer<vtkImageShiftScale> m_depthToUChar;
    vtkSmartPointer<vtkImageWriter> m_writer;
};
