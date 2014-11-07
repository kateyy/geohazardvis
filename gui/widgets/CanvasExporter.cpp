#include "CanvasExporter.h"
#include "ui_CanvasExporter.h"

#include <QDebug>
#include <QDateTime>
#include <QDir>

#include <vtkPNGWriter.h>
#include <vtkRenderWindow.h>
#include <vtkWindowToImageFilter.h>

#include <core/vtkhelper.h>
#include <gui/data_view/RenderView.h>


CanvasExporter::CanvasExporter(QWidget * parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_ui(new Ui_CanvasExporter)
    , m_exportFolder("screenshots")
{
    m_ui->setupUi(this);
}

CanvasExporter::~CanvasExporter()
{
    delete m_ui;
}

void CanvasExporter::setRenderView(RenderView * renderView)
{
    m_renderView = renderView;

    qDebug() << renderView;
}

void CanvasExporter::captureScreenshot()
{
    if (!m_renderView)
    {
        qDebug() << "CanvasExporter: not capturing screenshot, no render view set";
        return;
    }

    VTK_CREATE(vtkWindowToImageFilter, toImage);
    toImage->SetInput(m_renderView->renderWindow());
    toImage->SetInputBufferTypeToRGBA();
    toImage->ReadFrontBufferOff();

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH-mm-ss.zzzz");
    QString baseName = timestamp + " " + m_renderView->windowTitle();
    // http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247%28v=vs.85%29.aspx
    baseName.replace(QRegExp("[<>:\"/\\|?*]"), "_");

    QString exportFolder = m_exportFolder.isEmpty() ? "." : m_exportFolder;
    exportFolder.replace("\\", "//");
    QDir exportDir(exportFolder);

    if (!QDir().mkpath(exportFolder))
    {
        qDebug() << "CanvasExporter: cannot create output folder: " + exportFolder;
        return;
    }

    QString fileName = exportDir.absoluteFilePath(baseName);
    
    VTK_CREATE(vtkPNGWriter, writer);
    writer->SetFileName((fileName + ".png").toLatin1().data());
    writer->SetInputConnection(toImage->GetOutputPort());
    writer->Write();

    qDebug() << "image exported to: " << fileName;
}
