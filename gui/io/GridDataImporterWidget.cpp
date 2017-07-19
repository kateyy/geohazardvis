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

 #include "GridDataImporterWidget.h"
#include "ui_GridDataImporterWidget.h"

#include <array>
#include <cassert>

#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>

#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>

#include <core/data_objects/ImageDataObject.h>
#include <core/io/io_helper.h>
#include <core/io/MatricesToVtk.h>
#include <core/io/TextFileReader.h>
#include <core/utility/qthelper.h>


GridDataImporterWidget::GridDataImporterWidget(QWidget * parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , m_ui{ std::make_unique<Ui_GridDataImporterWidget>() }
    , m_importDataType{ VTK_FLOAT }
{
    m_ui->setupUi(this);

    connect(m_ui->imageFileOpenButton, &QAbstractButton::clicked, this, &GridDataImporterWidget::openImageDataFile);
    connect(m_ui->importButton, &QAbstractButton::clicked, [this] ()
    {
        if (importToImageData())
        {
            accept();
        }
    });
    connect(m_ui->cancelButton, &QAbstractButton::clicked, this, &GridDataImporterWidget::reject);
}

GridDataImporterWidget::~GridDataImporterWidget() = default;

std::unique_ptr<ImageDataObject> GridDataImporterWidget::releaseLoadedImageData()
{
    return std::move(m_imageData);
}

std::unique_ptr<DataObject> GridDataImporterWidget::releaseLoadedData()
{
    return std::move(m_imageData);
}

const QString & GridDataImporterWidget::openFileDirectory() const
{
    return m_lastOpenFolder;
}

void GridDataImporterWidget::setOpenFileDirectory(const QString & directory)
{
    m_lastOpenFolder = directory;
}

void GridDataImporterWidget::setImportDataType(int vtk_dataType)
{
    m_importDataType = vtk_dataType;
}

int GridDataImporterWidget::importDataType() const
{
    return m_importDataType;
}

void GridDataImporterWidget::openImageDataFile()
{
    const auto fileName = getOpenFileDialog("Open 2D Grid File");
    if (fileName.isEmpty())
    {
        return;
    }

    m_ui->imageFileEdit->setText(fileName);

    m_imageDataVector.clear();
    const auto result = TextFileReader(fileName).read(m_imageDataVector);
    if (!result.testFlag(TextFileReader::successful))
    {
        QMessageBox::warning(this, "Read Error", "Cannot open the specified file.");
        m_imageDataVector.clear();
    }

    updateSummary();
}

void GridDataImporterWidget::updateSummary()
{
    {
        QTableWidgetSetRowsWorker addRow{ *m_ui->summaryTable };
        if (!m_imageDataVector.empty())
        {
            addRow("Columns (X)", QString::number(m_imageDataVector.size()));
            addRow("Rows (Y)", QString::number(m_imageDataVector[0].size()));
        }
    }

    m_ui->summaryTable->resizeColumnsToContents();
}

bool GridDataImporterWidget::importToImageData()
{
    if (m_imageDataVector.empty())
    {
        QMessageBox::warning(this, "Import Failed", "Please specify valid input data first!");
        return false;
    }

    auto imageData = MatricesToVtk::parseGrid2D(m_imageDataVector, m_importDataType);
    assert(imageData);

    auto baseName = QFileInfo(m_ui->imageFileEdit->text()).baseName();

    bool ok;
    auto name = QInputDialog::getText(this, "Data Object Name", "Data Object Name", QLineEdit::Normal, baseName, &ok);
    if (name.isEmpty())
    {
        return false;
    }

    imageData->SetOrigin(m_ui->originXSpinBox->value(), m_ui->originYSpinBox->value(), 0);
    imageData->SetSpacing(m_ui->spacingXSpinBox->value(), m_ui->spacingYSpinBox->value(), 1);
    imageData->GetPointData()->GetScalars()->SetName(name.toUtf8().data());
    m_imageData = std::make_unique<ImageDataObject>(name, *imageData);

    return true;
}

void GridDataImporterWidget::clearData()
{
    io::InputVector().swap(m_imageDataVector);
    m_imageData.reset();
}

QString GridDataImporterWidget::getOpenFileDialog(const QString & title)
{
    auto && filters = io::fileFormatFilters(io::Category::CSV);

    const auto fileName = QFileDialog::getOpenFileName(this, title, m_lastOpenFolder, filters);
    if (fileName.isEmpty())
    {
        return{};
    }

    m_lastOpenFolder = QFileInfo(fileName).absolutePath();

    return fileName;
}

void GridDataImporterWidget::closeEvent(QCloseEvent * event)
{
    clearData();

    QDialog::closeEvent(event);
}
