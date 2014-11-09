#include "CanvasExporterPNG.h"

#include <QStringList>

#include <vtkPNGWriter.h>
#include <vtkRenderWindow.h>
#include <vtkWindowToImageFilter.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/canvas_export/CanvasExporterRegistry.h>


namespace
{
bool isRegistered = CanvasExporterRegistry::registerImplementation(CanvasExporter::newInstance<CanvasExporterPNG>);
}


CanvasExporterPNG::CanvasExporterPNG()
    : CanvasExporterImages()
    , m_writer(vtkSmartPointer<vtkPNGWriter>::New())
{
    m_writer->SetInputConnection(m_toImageFilter->GetOutputPort());
}

bool CanvasExporterPNG::write()
{
    m_toImageFilter->SetInput(renderWindow());
    m_writer->SetFileName(verifiedFileName().toLatin1().data());
    m_writer->Write();

    return true;
}

reflectionzeug::PropertyGroup * CanvasExporterPNG::createPropertyGroup()
{
    auto group = CanvasExporterImages::createPropertyGroup();

    auto prop_compression = group->addProperty<int>("CompressionLevel",
        [this] () { return m_writer->GetCompressionLevel(); },
        [this] (int level) { m_writer->SetCompressionLevel(level); }
    );
    prop_compression->setOption("title", "Compression Level");
    prop_compression->setOption("minimum", m_writer->GetCompressionLevelMinValue());
    prop_compression->setOption("maximum", m_writer->GetCompressionLevelMaxValue());

    return group;
}

QStringList CanvasExporterPNG::fileFormats() const
{
    return{ "PNG" };
}
