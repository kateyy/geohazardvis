#include "CanvasExporterPS.h"

#include <QStringList>

using namespace reflectionzeug;

#include "config.h"
#if VTK_module_IOExport

#include <vtkGL2PSExporter.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/canvas_export/CanvasExporterRegistry.h>



namespace
{
bool isRegistered = CanvasExporterRegistry::registerImplementation(CanvasExporter::newInstance<CanvasExporterPS>);
}

CanvasExporterPS::CanvasExporterPS()
    : CanvasExporter()
    , m_exporter(vtkSmartPointer<vtkGL2PSExporter>::New())
{
    m_exporter->CompressOff();
}

bool CanvasExporterPS::write()
{
    int vtkPSFormat = -1;
    if (outputFormat() == "PS")
        vtkPSFormat = vtkGL2PSExporter::PS_FILE;
    else if (outputFormat() == "EPS")
        vtkPSFormat = vtkGL2PSExporter::EPS_FILE;
    else if (outputFormat() == "PDF")
        vtkPSFormat = vtkGL2PSExporter::PDF_FILE;
    else if (outputFormat() == "TEX")
        vtkPSFormat = vtkGL2PSExporter::TEX_FILE;
    else if (outputFormat() == "SVG")
        vtkPSFormat = vtkGL2PSExporter::SVG_FILE;
    else
        return false;

    m_exporter->SetFilePrefix(outputFileName().toLatin1().data());
    m_exporter->SetFileFormat(vtkPSFormat);
    m_exporter->SetRenderWindow(renderWindow());
    m_exporter->Write();

    return true;
}

PropertyGroup * CanvasExporterPS::createPropertyGroup()
{
    PropertyGroup * group = new PropertyGroup();

    group->addProperty<std::string>("Title",
        [this] () -> std::string {
        std::string title;
        if (m_exporter->GetTitle())
            return m_exporter->GetTitle();
        else
            return ""; },
        [this] (const std::string & title) {
        m_exporter->SetTitle(title.c_str()); }
    );

    group->addProperty<bool>("Compress",
        [this] () { return m_exporter->GetCompress() != 0; },
        [this] (bool compress) { m_exporter->SetCompress(compress); }
    );

    auto prop_occlusionCull = group->addProperty<bool>("OcclusionCull",
        [this] () { return m_exporter->GetOcclusionCull() != 0; },
        [this] (bool compress) { m_exporter->SetOcclusionCull(compress); }
    );
    prop_occlusionCull->setOption("title", "remove occluded polygons");

    auto prop_textAsPath = group->addProperty<bool>("TextAsPath",
        [this] () { return m_exporter->GetTextAsPath() != 0; },
        [this] (bool compress) { m_exporter->SetTextAsPath(compress); }
    );
    prop_textAsPath->setOption("title", "Text as Path");

    return group;
}

QStringList CanvasExporterPS::fileFormats() const
{
    return{
        "PS", "EPS", "PDF", "TEX", "SVG"
    };
}


#else

CanvasExporterPS::CanvasExporterPS()
    : CanvasExporter()
{
}

bool CanvasExporterPS::write()
{
    return false;
}

PropertyGroup * CanvasExporterPS::createPropertyGroup()
{
    return nullptr;
}

QStringList CanvasExporterPS::fileFormats() const
{
    return{};
}
#endif
