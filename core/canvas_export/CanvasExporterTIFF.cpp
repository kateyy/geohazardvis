#include "CanvasExporterTIFF.h"

#include <QStringList>

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

const bool CanvasExporterTIFF::s_isRegistered = CanvasExporterRegistry::registerImplementation(CanvasExporter::newInstance<CanvasExporterTIFF>);


CanvasExporterTIFF::CanvasExporterTIFF()
    : CanvasExporterImages(vtkTIFFWriter::New())
{
}

CanvasExporterTIFF::~CanvasExporterTIFF() = default;

std::unique_ptr<reflectionzeug::PropertyGroup> CanvasExporterTIFF::createPropertyGroup()
{
    auto group = CanvasExporterImages::createPropertyGroup();

    vtkTIFFWriter * tiffWriter = static_cast<vtkTIFFWriter *>(m_writer.Get());

    auto prop_compression = group->addProperty<tiffCompression>("Compression",
        [tiffWriter] () { return static_cast<tiffCompression>(tiffWriter->GetCompression()); },
        [tiffWriter] (tiffCompression c) { tiffWriter->SetCompression(static_cast<int>(c)); }
    );
    prop_compression->setStrings({
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
