#include "CanvasExporterPNG.h"

#include <QStringList>

#include <vtkPNGWriter.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/canvas_export/CanvasExporterRegistry.h>


namespace
{
bool isRegistered = CanvasExporterRegistry::registerImplementation(CanvasExporter::newInstance<CanvasExporterPNG>);
}


CanvasExporterPNG::CanvasExporterPNG()
    : CanvasExporterImages(vtkPNGWriter::New())
{
}

std::unique_ptr<reflectionzeug::PropertyGroup> CanvasExporterPNG::createPropertyGroup()
{
    auto group = CanvasExporterImages::createPropertyGroup();

    vtkPNGWriter * pngWriter = static_cast<vtkPNGWriter *>(m_writer.Get());

    auto prop_compression = group->addProperty<int>("CompressionLevel",
        [pngWriter] () { return pngWriter->GetCompressionLevel(); },
        [pngWriter] (int level) { pngWriter->SetCompressionLevel(level); }
    );
    prop_compression->setOption("title", "Compression Level");
    prop_compression->setOption("minimum", pngWriter->GetCompressionLevelMinValue());
    prop_compression->setOption("maximum", pngWriter->GetCompressionLevelMaxValue());

    return group;
}

QStringList CanvasExporterPNG::fileFormats() const
{
    return{ "PNG" };
}
