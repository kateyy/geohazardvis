#include "CanvasExporterWidget.h"
#include "ui_CanvasExporterWidget.h"

#include <QDebug>
#include <QDateTime>
#include <QDir>

#include <propertyguizeug/PropertyBrowser.h>

#include <core/canvas_export/CanvasExporter.h>
#include <core/canvas_export/CanvasExporterRegistry.h>
#include <gui/data_view/RenderView.h>
#include <gui/propertyguizeug_extension/PropertyPainterEx.h>
#include <gui/propertyguizeug_extension/PropertyEditorFactoryEx.h>


using namespace propertyguizeug;

CanvasExporterWidget::CanvasExporterWidget(QWidget * parent, Qt::WindowFlags f)
    : QDialog(parent, f)
    , m_ui(new Ui_CanvasExporterWidget)
    , m_exporterSettingsBrowser(new PropertyBrowser(new PropertyEditorFactoryEx(), new PropertyPainterEx(), this))
{
    m_ui->setupUi(this);

    m_ui->exporterSettingsGroupBox->layout()->addWidget(m_exporterSettingsBrowser);

    connect(m_ui->fileFormatComboBox, &QComboBox::currentTextChanged, this, &CanvasExporterWidget::updateUiForFormat);

    m_ui->fileFormatComboBox->addItems(CanvasExporterRegistry::supportedFormatNames());

    m_ui->outputFolderEdit->setText("./screenshots");
}

CanvasExporterWidget::~CanvasExporterWidget()
{
    delete m_ui;
    qDeleteAll(m_exporters);
}

void CanvasExporterWidget::setRenderView(RenderView * renderView)
{
    m_renderView = renderView;
}

void CanvasExporterWidget::captureScreenshot()
{
    if (!m_renderView)
    {
        qDebug() << "CanvasExporterWidget: not capturing screenshot, no render view set";
        return;
    }

    QString format = m_ui->fileFormatComboBox->currentText();
    CanvasExporter * exporter = m_exporters.value(format, nullptr);
    if (!exporter)
    {
        exporter = CanvasExporterRegistry::createExporter(format);
        m_exporters.insert(format, exporter);
    }

    if (!exporter)
    {
        qDebug() << "CanvasExporterWidget: Selected file format" << format << "is not supported.";
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH-mm-ss.zzzz");
    QString baseName = timestamp + " " + m_renderView->windowTitle();
    // http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247%28v=vs.85%29.aspx
    baseName.replace(QRegExp("[<>:\"/\\|?*]"), "_");


    QString exportFolder = m_ui->outputFolderEdit->text().isEmpty() ? "." : m_ui->outputFolderEdit->text();
    exportFolder.replace("\\", "//");
    QDir exportDir(exportFolder);

    if (!QDir().mkpath(exportFolder))
    {
        qDebug() << "CanvasExporterWidget: cannot create output folder: " + exportFolder;
        return;
    }

    QString fileName = exportDir.absoluteFilePath(baseName);

    qDebug() << "exporting image to: " << fileName;

    exporter->setOutputFileName(fileName);
    exporter->setRenderWindow(m_renderView->renderWindow());

    exporter->write();
}

void CanvasExporterWidget::updateUiForFormat(const QString & format)
{
    m_exporterSettingsBrowser->setRoot(nullptr);

    CanvasExporter * exporter = m_exporters.value(format, nullptr);

    if (!exporter)
    {
        exporter = CanvasExporterRegistry::createExporter(format);
        m_exporters.insert(format, exporter);
    }

    if (exporter)
        m_exporterSettingsBrowser->setRoot(exporter->propertyGroup());
}
