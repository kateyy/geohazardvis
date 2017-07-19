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

#include "CanvasExporterPNG.h"

#include <QStringList>

#include <vtkNew.h>
#include <vtkPNGWriter.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/canvas_export/CanvasExporterRegistry.h>


const bool CanvasExporterPNG::s_isRegistered = CanvasExporterRegistry::registerImplementation(
    CanvasExporter::newInstance<CanvasExporterPNG>);


CanvasExporterPNG::CanvasExporterPNG()
    : CanvasExporterImages(vtkNew<vtkPNGWriter>().Get())
{
}

CanvasExporterPNG::~CanvasExporterPNG() = default;

std::unique_ptr<reflectionzeug::PropertyGroup> CanvasExporterPNG::createPropertyGroup()
{
    auto group = CanvasExporterImages::createPropertyGroup();

    auto pngWriter = static_cast<vtkPNGWriter *>(m_writer.Get());

    group->addProperty<int>("CompressionLevel",
        [pngWriter] () { return pngWriter->GetCompressionLevel(); },
        [pngWriter] (int level) { pngWriter->SetCompressionLevel(level); })
        ->setOptions({
            { "title", "Compression Level" },
            { "minimum", pngWriter->GetCompressionLevelMinValue() },
            { "maximum", pngWriter->GetCompressionLevelMaxValue() }
    });

    return group;
}

QStringList CanvasExporterPNG::fileFormats() const
{
    return{ "PNG" };
}
