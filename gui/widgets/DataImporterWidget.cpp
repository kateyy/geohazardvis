#include "DataImporterWidget.h"
#include "ui_DataImporterWidget.h"

#include <cassert>

#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>

#include <vtkPolyData.h>

#include <core/data_objects/PolyDataObject.h>
#include <core/io/TextFileReader.h>
#include <core/io/Loader.h>
#include <core/io/MatricesToVtk.h>


DataImporterWidget::DataImporterWidget(QWidget * parent, bool showNonCsvTypesTabs, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , m_ui{ std::make_unique<Ui_DataImporterWidget>() }
    , m_importDataType{ VTK_FLOAT }
{
    m_ui->setupUi(this);
    if (!showNonCsvTypesTabs)
    {
        while (m_ui->tabWidget->count() > 1)
        {
            m_ui->tabWidget->removeTab(1);
        }
    }

    connect(m_ui->tabWidget, &QTabWidget::currentChanged, [this] () {
        clearData();
        updateSummary();
    });

    connect(m_ui->coordsFileOpen, &QAbstractButton::clicked, this, &DataImporterWidget::openPointCoords);
    connect(m_ui->indicesFileOpen, &QAbstractButton::clicked, this, &DataImporterWidget::openTriangleIndices);
    connect(m_ui->polyDataFileOpen, &QAbstractButton::clicked, this, &DataImporterWidget::openPolyDataFile);
    connect(m_ui->importButton, &QAbstractButton::clicked, [this] () {
        if (m_ui->tabWidget->currentIndex() == 0)
        {
            if (importToPolyData())
            {
                accept();
            }
        }
        else
        {
            if (m_polyData)
            {
                accept();
            }
            else
            {
                QMessageBox::warning(this, "Import Failed", "Please specify a valid polygonal data file first!");
            }
        }
    });
    connect(m_ui->cancelButton, &QAbstractButton::clicked, this, &DataImporterWidget::reject);
}

DataImporterWidget::~DataImporterWidget() = default;

std::unique_ptr<PolyDataObject> DataImporterWidget::releaseLoadedPolyData()
{
    return std::move(m_polyData);
}

std::unique_ptr<DataObject> DataImporterWidget::releaseLoadedData()
{
    return std::move(m_polyData);
}

const QString & DataImporterWidget::openFileDirectory() const
{
    return m_lastOpenFolder;
}

void DataImporterWidget::setOpenFileDirectory(const QString & directory)
{
    m_lastOpenFolder = directory;
}

void DataImporterWidget::setImportDataType(int vtk_dataType)
{
    m_importDataType = vtk_dataType;
}

int DataImporterWidget::importDataType() const
{
    return m_importDataType;
}

void DataImporterWidget::openPointCoords()
{
    const auto fileName = getOpenFileDialog("Open Point Coordinates File", false);
    if (fileName.isEmpty())
    {
        return;
    }

    m_ui->coordsFileEdit->setText(fileName);

    m_coordinateData.clear();
    const auto result = TextFileReader(fileName).read(m_coordinateData);
    if (!result.testFlag(TextFileReader::successful))
    {
        QMessageBox::warning(this, "Read Error", "Cannot open the specified point coordinates file.");
    }

    updateSummary();
}

void DataImporterWidget::openTriangleIndices()
{
    const auto fileName = getOpenFileDialog("Open Triangle Indices File", false);
    if (fileName.isEmpty())
    {
        return;
    }

    m_ui->indicesFileEdit->setText(fileName);

    m_indexData.clear();
    const auto result = TextFileReader(fileName).read(m_indexData);
    if (!result.testFlag(TextFileReader::successful))
    {
        QMessageBox::warning(this, "Read Error", "Cannot open the specified triangle index file.");
    }

    updateSummary();
}

void DataImporterWidget::openPolyDataFile()
{
    const auto fileName = getOpenFileDialog("Open Polygonal Data File", true);
    if (fileName.isEmpty())
    {
        return;
    }

    m_ui->polyDataFileEdit->setText(fileName);

    m_coordinateData.clear();
    m_indexData.clear();
    m_polyData.reset();

    auto loadedData = Loader::readFile<PolyDataObject>(fileName);
    if (!loadedData)
    {
        QMessageBox::warning(this, "Read Error", "Cannot open the specified polygonal data file.");
    }
    else
    {
        loadedData->clearAttributes();
        m_polyData = std::move(loadedData);
    }

    updateSummary();
}

void DataImporterWidget::updateSummary()
{
    int rowCount = 0;
    vtkIdType numCoordinates = -1, numTriangles = -1, numAttributeColumns = -1;
    if (m_polyData)
    {
        rowCount = 2;
        numCoordinates = m_polyData->dataSet()->GetNumberOfPoints();
        numTriangles = m_polyData->dataSet()->GetNumberOfCells();
    }
    else
    {
        if (!m_coordinateData.empty())
        {
            rowCount += 1;
            numCoordinates = static_cast<vtkIdType>(m_coordinateData.front().size());
        }
        if (!m_indexData.empty())
        {
            rowCount += 2;
            numTriangles = static_cast<vtkIdType>(m_indexData.front().size());
            numAttributeColumns = static_cast<vtkIdType>(m_indexData.size()) - 3;
        }
    }

    m_ui->summaryTable->setRowCount(rowCount);

    int currentRow = 0;

    if (numCoordinates >= 0)
    {
        m_ui->summaryTable->setItem(currentRow, 0,
            new QTableWidgetItem("Point coordinates"));
        m_ui->summaryTable->setItem(currentRow, 1,
            new QTableWidgetItem(QString::number(numCoordinates)));
        ++currentRow;
    }

    if (numTriangles >= 0)
    {
        m_ui->summaryTable->setItem(currentRow, 0,
            new QTableWidgetItem("Triangles"));
        m_ui->summaryTable->setItem(currentRow, 1,
            new QTableWidgetItem(QString::number(numTriangles)));
        ++currentRow;
    }

    if (numAttributeColumns >= 0)
    {
        m_ui->summaryTable->setItem(currentRow, 0,
            new QTableWidgetItem("Attributes columns per triangle (ignored)"));
        m_ui->summaryTable->setItem(currentRow, 1,
            new QTableWidgetItem(QString::number(numAttributeColumns)));
        ++currentRow;
    }

    m_ui->summaryTable->resizeColumnsToContents();
}

bool DataImporterWidget::importToPolyData()
{
    if (m_coordinateData.empty() || m_indexData.empty())
    {
        QMessageBox::warning(this, "Import Failed", "Please specify valid input data first!");
        return false;
    }

    auto polyData = MatricesToVtk::parseIndexedTriangles(
        m_coordinateData, 0, 1,
        m_indexData, 0,
        m_importDataType);
    assert(polyData);

    auto baseName = QFileInfo(m_ui->indicesFileEdit->text()).baseName();

    bool ok;
    auto name = QInputDialog::getText(this, "Data Object Name", "Data Object Name", QLineEdit::Normal, baseName, &ok);
    if (name.isEmpty())
    {
        return false;
    }

    m_polyData = std::make_unique<PolyDataObject>(name, *polyData);

    return true;
}

void DataImporterWidget::clearData()
{
    io::InputVector().swap(m_coordinateData);
    io::InputVector().swap(m_indexData);
    m_polyData.reset();
}

QString DataImporterWidget::getOpenFileDialog(const QString & title, bool requestPolyData)
{
    auto && filters = Loader::fileFormatFilters(requestPolyData
        ? Loader::Category::PolyData
        : Loader::Category::CSV);

    const auto fileName = QFileDialog::getOpenFileName(this, title, m_lastOpenFolder, filters);
    if (fileName.isEmpty())
    {
        return{};
    }

    m_lastOpenFolder = QFileInfo(fileName).absolutePath();

    return fileName;
}

void DataImporterWidget::closeEvent(QCloseEvent * event)
{
    clearData();

    QDialog::closeEvent(event);
}
