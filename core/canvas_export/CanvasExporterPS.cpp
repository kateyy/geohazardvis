#include "CanvasExporterPS.h"

#include <cassert>

#include <QFileInfo>
#include <QStringList>

#include <reflectionzeug/PropertyGroup.h>

#include <vtkGL2PSExporter.h>

#include <core/config.h>
#if WIN32 && VTK_RENDERING_BACKEND == 1
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLExtensionManager.h>
#endif

#include <core/canvas_export/CanvasExporterRegistry.h>


const bool CanvasExporterPS::s_isRegistered = CanvasExporterRegistry::registerImplementation(CanvasExporter::newInstance<CanvasExporterPS>);


CanvasExporterPS::CanvasExporterPS()
    : CanvasExporter()
    , m_exporter{ vtkSmartPointer<vtkGL2PSExporter>::New() }
{
    m_exporter->CompressOff();
}

CanvasExporterPS::~CanvasExporterPS() = default;

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
    {
        fileName.truncate(fileName.length() - ext.length() - 1); // cut ".EXT"
    }

    m_exporter->SetFilePrefix(fileName.toUtf8().data());
    m_exporter->SetFileFormat(vtkPSFormat);
    m_exporter->SetRenderWindow(renderWindow());
    m_exporter->Write();

    return true;
}

bool CanvasExporterPS::openGLContextSupported()
{
#if WIN32 && VTK_RENDERING_BACKEND == 1
    auto openGLContext = vtkOpenGLRenderWindow::SafeDownCast(renderWindow());
    assert(openGLContext);
    auto extensionManager = openGLContext->GetExtensionManager();

    if (extensionManager->DriverIsIntel())
    {
        return false;
    }
#endif

    return CanvasExporter::openGLContextSupported();
}

std::unique_ptr<reflectionzeug::PropertyGroup> CanvasExporterPS::createPropertyGroup()
{
    auto group = std::make_unique<reflectionzeug::PropertyGroup>();

    group->addProperty<std::string>("Title",
        [this] () -> std::string {
        if (auto title = m_exporter->GetTitle())
        {
            return title;
        }
        return "";
    },
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
    {
        f = format.arg(f);
    }

    return formats;
}
