#include "CanvasExporterPS.h"

#include <QFileInfo>
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

QString CanvasExporterPS::fileExtension() const
{
    QString format = outputFormat();
    int prefixLength = QString("PostScript (").length();
    int length = format.length() - prefixLength - 1;

    QStringRef ext(&format, prefixLength, length);

    return ext.toString().toLower();
}

bool CanvasExporterPS::write()
{
    QString format("PostScript (%1)");

    int vtkPSFormat = -1;
    if (outputFormat() == format.arg("PS"))
        vtkPSFormat = vtkGL2PSExporter::PS_FILE;
    else if (outputFormat() == format.arg("EPS"))
        vtkPSFormat = vtkGL2PSExporter::EPS_FILE;
    else if (outputFormat() == format.arg("PDF"))
        vtkPSFormat = vtkGL2PSExporter::PDF_FILE;
    else if (outputFormat() == format.arg("TEX"))
        vtkPSFormat = vtkGL2PSExporter::TEX_FILE;
    else if (outputFormat() == format.arg("SVG"))
        vtkPSFormat = vtkGL2PSExporter::SVG_FILE;
    else
        return false;

    // vtkGL2PSExporter always appends a file extensions, so we should cut it if it already exists
    QString fileName = outputFileName();
    QString ext = fileExtension();
    QFileInfo info(fileName);
    if (info.suffix().toLower() == ext)
        fileName.truncate(fileName.length() - ext.length() - 1); // cut ".EXT"

    m_exporter->SetFilePrefix(fileName.toLatin1().data());
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
    QString format("PostScript (%1)");

    QStringList formats {
        "PS", "EPS", "PDF", "TEX", "SVG"
    };

    for (QString & f : formats)
        f = format.arg(f);

    return formats;
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
