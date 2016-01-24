#include <gui/widgets/AsciiImporterWidget.h>
#include "ui_AsciiImporterWidget.h"

#include <cassert>

#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>

#include <vtkPolyData.h>

#include <core/data_objects/PolyDataObject.h>
#include <core/io/FileParser.h>
#include <core/io/MatricesToVtk.h>


AsciiImporterWidget::AsciiImporterWidget(QWidget * parent, Qt::WindowFlags f)
    : QDialog(parent, f)
    , m_ui(std::make_unique<Ui_AsciiImporterWidget>())
    , m_importDataType(VTK_FLOAT)
{
    m_ui->setupUi(this);
    connect(m_ui->coordsFileOpen, &QAbstractButton::clicked, this, &AsciiImporterWidget::openPointCoords);
    connect(m_ui->indicesFileOpen, &QAbstractButton::clicked, this, &AsciiImporterWidget::openTriangleIndices);
    connect(m_ui->importButton, &QAbstractButton::clicked, [this] () {
        if (importToPolyData())
            accept();
    });
    connect(m_ui->cancelButton, &QAbstractButton::clicked, this, &AsciiImporterWidget::reject);
}

AsciiImporterWidget::~AsciiImporterWidget() = default;

std::unique_ptr<PolyDataObject> AsciiImporterWidget::releaseLoadedPolyData()
{
    return std::move(m_polyData);
}

std::unique_ptr<DataObject> AsciiImporterWidget::releaseLoadedData()
{
    return std::unique_ptr<DataObject>(m_polyData.release());
}

const QString & AsciiImporterWidget::openFileDirectory() const
{
    return m_lastOpenFolder;
}

void AsciiImporterWidget::setOpenFileDirectory(const QString & directory)
{
    m_lastOpenFolder = directory;
}

void AsciiImporterWidget::setImportDataType(int vtk_dataType)
{
    m_importDataType = vtk_dataType;
}

int AsciiImporterWidget::importDataType() const
{
    return m_importDataType;
}

void AsciiImporterWidget::openPointCoords()
{
    auto fileName = getOpenFileDialog("Open Point Coordinates File");
    if (fileName.isEmpty())
    {
        return;
    }

    m_ui->coordsFileEdit->setText(fileName);
    
    m_coordinateData.clear();
    if (!FileParser::populateIOVectors(fileName.toStdString(), m_coordinateData))
    {
        QMessageBox::warning(this, "Read Error", "Cannot open the specified point coordinates file.");
    }

    updateSummary();
}

void AsciiImporterWidget::openTriangleIndices()
{
    auto fileName = getOpenFileDialog("Open Triangle Indices File");
    if (fileName.isEmpty())
    {
        return;
    }

    m_ui->indicesFileEdit->setText(fileName);

    m_indexData.clear();
    if (!FileParser::populateIOVectors(fileName.toStdString(), m_indexData))
    {
        QMessageBox::warning(this, "Read Error", "Cannot open the specified point coordinates file.");
    }

    updateSummary();
}

void AsciiImporterWidget::updateSummary()
{
    int rowCount = 0;
    if (!m_coordinateData.empty())
    {
        rowCount += 1;
    }
    if (!m_indexData.empty())
    {
        rowCount += 2;
    }

    m_ui->summaryTable->setRowCount(rowCount);

    int currentRow = 0;

    if (!m_coordinateData.empty())
    {
        m_ui->summaryTable->setItem(currentRow, 0, 
            new QTableWidgetItem("Point coordinates"));
        m_ui->summaryTable->setItem(currentRow, 1, 
            new QTableWidgetItem(QString::number(m_coordinateData.front().size())));
        ++currentRow;
    }

    if (!m_indexData.empty())
    {
        m_ui->summaryTable->setItem(currentRow, 0,
            new QTableWidgetItem("Triangles"));
        m_ui->summaryTable->setItem(currentRow, 1,
            new QTableWidgetItem(QString::number(m_indexData.front().size())));
        ++currentRow;

        m_ui->summaryTable->setItem(currentRow, 0,
            new QTableWidgetItem("Attributes columns per triangle (ignored)"));
        m_ui->summaryTable->setItem(currentRow, 1,
            new QTableWidgetItem(QString::number(m_indexData.size() - 3)));
        ++currentRow;
    }

    m_ui->summaryTable->resizeColumnsToContents();
}

bool AsciiImporterWidget::importToPolyData()
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
        QMessageBox::information(this, "Import Failed", "Please set a name for the new imported data object!");
        return false;
    }

    m_polyData = std::make_unique<PolyDataObject>(name, *polyData);

    return true;
}

void AsciiImporterWidget::clearData()
{
    io::InputVector().swap(m_coordinateData);
    io::InputVector().swap(m_indexData);
}

QString AsciiImporterWidget::getOpenFileDialog(const QString & title)
{
    auto fileName = QFileDialog::getOpenFileName(this, title, m_lastOpenFolder, "ASCII text files (*.txt);;All Files (*)");
    if (fileName.isEmpty())
    {
        return{};
    }

    m_lastOpenFolder = QFileInfo(fileName).absolutePath();

    return fileName;
}

void AsciiImporterWidget::closeEvent(QCloseEvent * event)
{
    clearData();

    QWidget::closeEvent(event);
}
