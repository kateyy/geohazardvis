/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CanvasExporterWidget.h"
#include "ui_CanvasExporterWidget.h"

#include <algorithm>
#include <cassert>

#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

#include <core/canvas_export/CanvasExporter.h>
#include <core/canvas_export/CanvasExporterRegistry.h>
#include <core/io/io_helper.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/propertyguizeug_extension/ColorEditorRGB.h>


CanvasExporterWidget::CanvasExporterWidget(QWidget * parent, Qt::WindowFlags f)
    : QDialog(parent, f)
    , m_ui{ std::make_unique<Ui_CanvasExporterWidget>() }
    , m_renderView{ nullptr }
{
    m_ui->setupUi(this);
    m_ui->exporterSettingsBrowser->addEditorPlugin<ColorEditorRGB>();
    m_ui->exporterSettingsBrowser->addPainterPlugin<ColorEditorRGB>();

    connect(m_ui->fileFormatComboBox, &QComboBox::currentTextChanged, this, &CanvasExporterWidget::updateUiForFormat);

    auto && formats = CanvasExporterRegistry::supportedFormatNames();
    m_ui->fileFormatComboBox->addItems(formats);
    const auto pngIt = std::find(formats.begin(), formats.end(), "PNG");
    if (pngIt != formats.end())
    {
        m_ui->fileFormatComboBox->setCurrentIndex(pngIt - formats.begin());
    }

    m_ui->outputFolderEdit->setText("./screenshots");
    connect(m_ui->outputFolderButton, &QPushButton::clicked, [this] () {
        QString res = QFileDialog::getExistingDirectory(this, "Screenshot Output Folder",
            m_ui->outputFolderEdit->text());
        if (!res.isEmpty())
        {
            m_ui->outputFolderEdit->setText(res);
        }
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
    {
        return;
    }

    const QString exportFolder = currentExportFolder();

    const QDir exportDir(exportFolder);

    if (!exportDir.exists())
    {
        return;
    }

    const QString fileName = exportDir.absoluteFilePath(fileNameWithTimeStamp());

    saveScreenshotTo(*exporter, fileName);
}

void CanvasExporterWidget::captureScreenshotTo()
{
    CanvasExporter * exporter = currentExporterConfigured();
    if (!exporter)
    {
        return;
    }

    const QString target = QFileDialog::getSaveFileName(this, "Export Image",
        currentExportFolder() + "/" + fileNameWithTimeStamp(),
        exporter->outputFormat() + " (*." + exporter->fileExtension() + ")");

    if (target.isEmpty())
    {
        return;
    }

    saveScreenshotTo(*exporter, target);
}

CanvasExporter * CanvasExporterWidget::currentExporter()
{
    const QString format = m_ui->fileFormatComboBox->currentText();
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
    {
        qDebug() << "CanvasExporterWidget: Selected file format" << format << "is not supported.";
    }

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

        if (!exporter->openGLContextSupported())
        {
            QMessageBox::warning(this, "Unsupported OpenGL Driver",
                QString("Unfortunately, the currently selected image export format is not supported with your graphics driver.") +
                "\nExport format: " + m_ui->fileFormatComboBox->currentText());
            return nullptr;
        }
    }

    return exporter;
}

QString CanvasExporterWidget::currentExportFolder() const
{
    QString exportFolder = m_ui->outputFolderEdit->text().isEmpty() ? "." : m_ui->outputFolderEdit->text();
    exportFolder.replace("\\", "//");

    if (!QDir().mkpath(exportFolder))
    {
        qDebug() << "CanvasExporterWidget: cannot create output folder: " + exportFolder;
    }

    return exportFolder;
}

QString CanvasExporterWidget::fileNameWithTimeStamp() const
{
    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH-mm-ss.zzzz");
    QString baseName = timestamp + " " + m_renderView->windowTitle();
    baseName = io::normalizeFileName(baseName);
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
    {
        return;
    }

    m_ui->exporterSettingsBrowser->setRoot(exporter->propertyGroup());
    m_ui->exporterSettingsBrowser->resizeColumnToContents(0);
}
