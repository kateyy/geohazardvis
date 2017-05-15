#include "CanvasExporterJPEG.h"

#include <QStringList>

#include <vtkJPEGWriter.h>
#include <vtkNew.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/canvas_export/CanvasExporterRegistry.h>


const bool CanvasExporterJPEG::s_isRegistered = CanvasExporterRegistry::registerImplementation(CanvasExporter::newInstance<CanvasExporterJPEG>);


CanvasExporterJPEG::CanvasExporterJPEG()
    : CanvasExporterImages(vtkNew<vtkJPEGWriter>().Get())
{
}

CanvasExporterJPEG::~CanvasExporterJPEG() = default;

std::unique_ptr<reflectionzeug::PropertyGroup> CanvasExporterJPEG::createPropertyGroup()
{
    auto group = CanvasExporterImages::createPropertyGroup();

    auto jpgWriter = static_cast<vtkJPEGWriter *>(m_writer.Get());

    group->addProperty<int>("Quality",
        [jpgWriter] () { return jpgWriter->GetQuality(); },
        [jpgWriter] (int q) { jpgWriter->SetQuality(q); })
        ->setOptions({
            { "minimum", jpgWriter->GetQualityMinValue() },
            { "maximum", jpgWriter->GetQualityMaxValue() }
    });

    group->addProperty<bool>("Progressive",
        [jpgWriter] () { return jpgWriter->GetProgressive() != 0; },
        [jpgWriter] (bool value) { jpgWriter->SetProgressive(value); });

    return group;
}

QStringList CanvasExporterJPEG::fileFormats() const
{
    return{ "JPEG" };
}
