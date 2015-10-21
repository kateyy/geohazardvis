#include "CanvasExporterWidget.h"
#include "ui_CanvasExporterWidget.h"

#include <cassert>

#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>

#include <core/canvas_export/CanvasExporter.h>
#include <core/canvas_export/CanvasExporterRegistry.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/propertyguizeug_extension/ColorEditorRGB.h>


CanvasExporterWidget::CanvasExporterWidget(QWidget * parent, Qt::WindowFlags f)
    : QDialog(parent, f)
    , m_ui(std::make_unique<Ui_CanvasExporterWidget>())
    , m_renderView(nullptr)
{
    m_ui->setupUi(this);
    m_ui->exporterSettingsBrowser->addEditorPlugin<ColorEditorRGB>();
    m_ui->exporterSettingsBrowser->addPainterPlugin<ColorEditorRGB>();

    connect(m_ui->fileFormatComboBox, &QComboBox::currentTextChanged, this, &CanvasExporterWidget::updateUiForFormat);

    m_ui->fileFormatComboBox->addItems(CanvasExporterRegistry::supportedFormatNames());

    m_ui->outputFolderEdit->setText("./screenshots");
    connect(m_ui->outputFolderButton, &QPushButton::clicked, [this] () {
        QString res = QFileDialog::getExistingDirectory(this, "Screenshot Output Folder",
            m_ui->outputFolderEdit->text());
        if (!res.isEmpty())
            m_ui->outputFolderEdit->setText(res);
    });
}

CanvasExporterWidget::~CanvasExporterWidget() = default;

void CanvasExporterWidget::setRenderView(AbstractRenderView * renderView)
{
    m_renderView = renderView;
}

void CanvasExporterWidget::captureScreenshot()
{
    CanvasExporter * exporter = currentExporterConfigured();
    if (!exporter)
        return;

    QString exportFolder = currentExportFolder();

    QDir exportDir(exportFolder);

    if (!exportDir.exists())
        return;

    QString fileName = exportDir.absoluteFilePath(fileNameWithTimeStamp());

    saveScreenshotTo(*exporter, fileName);
}

void CanvasExporterWidget::captureScreenshotTo()
{
    CanvasExporter * exporter = currentExporterConfigured();
    if (!exporter)
        return;

    QString target = QFileDialog::getSaveFileName(this, "Export Image", 
        currentExportFolder() + "/" + fileNameWithTimeStamp(),
        exporter->outputFormat() + " (*." + exporter->fileExtension() + ")");

    if (target.isEmpty())
        return;

    saveScreenshotTo(*exporter, target);
}

CanvasExporter * CanvasExporterWidget::currentExporter()
{
    QString format = m_ui->fileFormatComboBox->currentText();
    auto exporterIt = m_exporters.find(format);

    CanvasExporter * exporter = nullptr;

    if (exporterIt == m_exporters.end())
    {
        auto newExporter = CanvasExporterRegistry::createExporter(format);
        exporter = newExporter.get();
        m_exporters.emplace(format, std::move(newExporter));
    }
    else
    {
        exporter = exporterIt->second.get();
    }

    if (!exporter)
        qDebug() << "CanvasExporterWidget: Selected file format" << format << "is not supported.";

    return exporter;
}

CanvasExporter * CanvasExporterWidget::currentExporterConfigured()
{
    if (!m_renderView)
    {
        qDebug() << "CanvasExporterWidget: not capturing screenshot, no render view set";
        return nullptr;
    }

    auto exporter = currentExporter();
    if (exporter)
    {
        exporter->setRenderWindow(m_renderView->renderWindow());
    }

    return exporter;
}

QString CanvasExporterWidget::currentExportFolder() const
{
    QString exportFolder = m_ui->outputFolderEdit->text().isEmpty() ? "." : m_ui->outputFolderEdit->text();
    exportFolder.replace("\\", "//");

    if (!QDir().mkpath(exportFolder))
        qDebug() << "CanvasExporterWidget: cannot create output folder: " + exportFolder;

    return exportFolder;
}

QString CanvasExporterWidget::fileNameWithTimeStamp() const
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH-mm-ss.zzzz");
    QString baseName = timestamp + " " + m_renderView->windowTitle();
    // http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247%28v=vs.85%29.aspx
    baseName.replace(QRegExp("[<>:\"/\\|?*]"), "_");

    return baseName;
}

void CanvasExporterWidget::saveScreenshotTo(CanvasExporter & exporter, const QString & fileName) const
{
    qDebug() << "exporting image to: " << fileName;

    exporter.setOutputFileName(fileName);
    exporter.setRenderWindow(m_renderView->renderWindow());

    exporter.write();
}

void CanvasExporterWidget::updateUiForFormat()
{
    m_ui->exporterSettingsBrowser->setRoot(nullptr);

    auto exporter = currentExporter();
    if (!exporter)
        return;

    m_ui->exporterSettingsBrowser->setRoot(exporter->propertyGroup());
    m_ui->exporterSettingsBrowser->resizeColumnToContents(0);
}
