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
#include <core/io/TextFileReader.h>
#include <core/io/Loader.h>
#include <core/io/MatricesToVtk.h>


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
    const auto result = TextFileReader::read(fileName, m_imageDataVector);
    if (!result.stateFlags.testFlag(TextFileReader::Result::successful))
    {
        QMessageBox::warning(this, "Read Error", "Cannot open the specified file.");
    }

    updateSummary();
}

void GridDataImporterWidget::updateSummary()
{
    int rowCount = 0;
    std::array<size_t, 2> dimensions;

    if (!m_imageDataVector.empty())
    {
        dimensions = { m_imageDataVector.size(), m_imageDataVector.at(0).size() };
        rowCount = 2;
    }

    m_ui->summaryTable->setRowCount(rowCount);

    if (rowCount >= 2)
    {
        m_ui->summaryTable->setItem(0, 0,
            new QTableWidgetItem("Columns (X)"));
        m_ui->summaryTable->setItem(0, 1,
            new QTableWidgetItem(QString::number(dimensions[0])));
        m_ui->summaryTable->setItem(1, 0,
            new QTableWidgetItem("Rows (Y)"));
        m_ui->summaryTable->setItem(1, 1,
            new QTableWidgetItem(QString::number(dimensions[1])));
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
    auto && filters = Loader::fileFormatFilters(Loader::Category::CSV);

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
