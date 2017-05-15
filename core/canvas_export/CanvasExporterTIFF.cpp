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
