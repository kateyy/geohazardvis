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

#include "CanvasExporterTIFF.h"

#include <QStringList>

#include <vtkNew.h>
#include <vtkTIFFWriter.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/canvas_export/CanvasExporterRegistry.h>


namespace
{

enum tiffCompression
{
    NoCompression = vtkTIFFWriter::NoCompression,
    PackBits = vtkTIFFWriter::PackBits,
    JPEG = vtkTIFFWriter::JPEG,
    Deflate = vtkTIFFWriter::Deflate,
    LZW = vtkTIFFWriter::LZW
};
}

const bool CanvasExporterTIFF::s_isRegistered = CanvasExporterRegistry::registerImplementation(
    CanvasExporter::newInstance<CanvasExporterTIFF>);


CanvasExporterTIFF::CanvasExporterTIFF()
    : CanvasExporterImages(vtkNew<vtkTIFFWriter>().Get())
{
}

CanvasExporterTIFF::~CanvasExporterTIFF() = default;

std::unique_ptr<reflectionzeug::PropertyGroup> CanvasExporterTIFF::createPropertyGroup()
{
    auto group = CanvasExporterImages::createPropertyGroup();

    auto tiffWriter = static_cast<vtkTIFFWriter *>(m_writer.Get());

    group->addProperty<tiffCompression>("Compression",
        [tiffWriter] () { return static_cast<tiffCompression>(tiffWriter->GetCompression()); },
        [tiffWriter] (tiffCompression c) { tiffWriter->SetCompression(static_cast<int>(c)); })
        ->setStrings({
            { NoCompression, "No Compression" },
            { PackBits, "Pack Bits" },
            { JPEG, "JPEG" },
            { Deflate, "Deflate" },
            { LZW, "LZW" }
    });

    return group;
}

QStringList CanvasExporterTIFF::fileFormats() const
{
    return{ "TIFF" };
}
