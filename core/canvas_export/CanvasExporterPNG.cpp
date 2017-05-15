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
