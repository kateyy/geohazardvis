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

#include "DataImporterWidget.h"
#include "ui_DataImporterWidget.h"

#include <cassert>
#include <set>
#include <limits>
#include <utility>
#include <vector>

#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>

#include <vtkInformation.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>

#include <core/CoordinateSystems.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/io/io_helper.h>
#include <core/io/Loader.h>
#include <core/io/MatricesToVtk.h>
#include <core/io/TextFileReader.h>
#include <core/utility/qthelper.h>
#include <core/utility/vtkstringhelper.h>


namespace
{

template<typename Data_t>
auto read(const QString & fileName, const QString & delimiterStr, Data_t & data)
    -> decltype(std::declval<TextFileReader>().read(data))
{
    auto reader = TextFileReader(fileName);
    reader.setDelimiter(delimiterStr.isEmpty() ? QChar(' ') : delimiterStr[0]);
    return reader.read(data);
}

bool parseColumns(const QString & spec, size_t numDataColumns, std::vector<int> & parts)
{
    QRegExp rx{ "[\\s;,]+" };
    const auto refs = spec.splitRef(rx, QString::SkipEmptyParts);
    for (auto && part : refs)
    {
        bool okay = false;
        const int column = part.toInt(&okay);
        if (!okay)
        {
            return false;
        }
        if (column < 1 || static_cast<size_t>(column) > numDataColumns)
        {
            return false;
        }
        parts.push_back(column);
    }
    return true;
}

}


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

    /** ==== Point Attribute Table UI events  ==== */

    updatePointFileAttributesTable();
    m_ui->pointAttributesTable->resizeColumnsToContents();
    connect(m_ui->pointAttributesTable, &QTableWidget::itemChanged, 
        this, &DataImporterWidget::updatePointFileAttributesTable);
}

DataImporterWidget::~DataImporterWidget() = default;

std::unique_ptr<GenericPolyDataObject> DataImporterWidget::releaseLoadedPolyData()
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
    const auto result = read(fileName, m_ui->delimiterEdit->text(), m_coordinateData);
    if (!result.testFlag(TextFileReader::successful))
    {
        QMessageBox::warning(this, "Read Error", "Cannot open the specified point coordinates file.");
    }

    updateSummary();
    updatePointFileAttributesTable();
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
    const auto result = read(fileName, m_ui->delimiterEdit->text(), m_indexData);
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
    {
        QTableWidgetSetRowsWorker addRow{ *m_ui->summaryTable };

        if (m_polyData)
        {
            addRow("Data Set Type", "Triangular Mesh");
            addRow("Point Coordinates",
                QString::number(m_polyData->dataSet()->GetNumberOfPoints()));
            addRow("Triangles", QString::number(m_polyData->dataSet()->GetNumberOfCells()));
        }
        else
        {
            QString dataSetType = "unknown";
            if (!m_coordinateData.empty())
            {
                dataSetType = "Point Cloud";
                addRow("Point Coordinates", QString::number(m_coordinateData.front().size()));
            }
            if (!m_indexData.empty())
            {
                dataSetType = "Triangular Mesh";
                addRow("Triangles", QString::number(m_indexData.front().size()));
                addRow("Attributes columns per triangle (ignored)",
                    QString::number(m_indexData.size() - 3));
            }
            addRow("Data Set Type", dataSetType, 0);
        }
    }

    m_ui->summaryTable->resizeColumnsToContents();
}

bool DataImporterWidget::importToPolyData()
{
    if (m_coordinateData.empty())
    {
        QMessageBox::warning(this, "Import Failed", "Please specify valid input data first!");
        return false;
    }

    // Find point coordinate column specification

    std::set<QString> usedAttributeNames;
    std::set<int> usedColumns;
    struct Attribute
    {
        QString name;
        std::vector<int> columns;
        QString unit;
    };
    Attribute coordsAttribute;
    coordsAttribute.name = "Points";
    std::vector<Attribute> pointAttributes;

    for (int i = 0; i < m_ui->pointAttributesTable->rowCount() - 2; ++i)
    {
        auto nameItem = m_ui->pointAttributesTable->item(i, 0);
        auto specItem = m_ui->pointAttributesTable->item(i, 1);
        auto unitItem = m_ui->pointAttributesTable->item(i, 2);
        const auto name = nameItem ? nameItem->data(Qt::DisplayRole).toString() : QString();
        const auto spec = specItem ? specItem->data(Qt::DisplayRole).toString() : QString();
        const auto unit = unitItem ? unitItem->data(Qt::DisplayRole).toString() : QString();
        if (name.isEmpty() && spec.isEmpty())
        {
            continue;
        }
        if (name.isEmpty())
        {
            QMessageBox::information(this, "Point Attributes",
                QString("The point attribute in row %1 does not have a name. "
                    "Please set one or remove the line.").arg(i + 1));
            return false;
        }
        if (!isStringIsAcceptable(name))
        {
            QMessageBox::information(this, "Point Attributes",
                QString("The point attribute name \"%1\" contains unsupported characters. "
                    "Please note that some characters such as the degree sign (%2) can currently "
                    "not be used.").arg(name).arg(QString(QChar(0x00B0))));
            return false;
        }
        if (spec.isEmpty())
        {
            QMessageBox::information(this, "Point Attributes",
                QString("The point attribute \"%1\" in row %2 has no columns associated. "
                    "Please set at least one or remove the line.").arg(name).arg(i + 1));
            return false;
        }
        if (!isStringIsAcceptable(unit))
        {
            QMessageBox::information(this, "Point Attributes",
                QString("The unit symbol \"%1\" contains unsupported characters. "
                    "Please note that some characters such as the degree sign (%2) can currently "
                    "not be used.").arg(unit).arg(QString(QChar(0x00B0))));
            return false;
        }
        std::vector<int> columns;
        if (!parseColumns(spec, m_coordinateData.size(), columns))
        {
            QMessageBox::information(this, "Point Attributes",
                QString("The point attribute \"%1\" in row %2 has an invalid column specification: %3. "
                    "Please enter a list of positive numbers between one and the number of columns "
                    "in your input file, separated by comma, semicolon or whitespace.")
                .arg(name).arg(i + 1).arg(spec));
            return false;
        }
        if (!usedAttributeNames.emplace(name).second)
        {
            QMessageBox::information(this, "Point Attributes",
                QString("The attribute %1 was found multiple times in the table. "
                    "Please provide only one set of columns per attribute name.").arg(name));
            return false;
        }
        for (int column : columns)
        {
            if (!usedColumns.emplace(column).second)
            {
                QMessageBox::information(this, "Point Attributes",
                    QString("The file column %1 was specific for more than one attribute. "
                        "Please use every column only once.").arg(column));
                return false;
            }
        }
        if (name == coordsAttribute.name)
        {
            if (columns.size() != 2 && columns.size() != 3)
            {
                QMessageBox::information(this, "Point Attributes",
                    QString("You specified %1 data columns for point coordinates, but 2 (for x, y) or "
                        "3 (for x, y, z) are required.").arg(columns.size()));
                return false;
            }
            coordsAttribute.columns = std::move(columns);
            coordsAttribute.unit = std::move(unit);
        }
        else
        {
            pointAttributes.emplace_back(Attribute{
                std::move(name),
                std::move(columns),
                std::move(unit) });
        }
    }
    if (coordsAttribute.columns.empty())
    {
        QMessageBox::information(this, "Point Attributes",
            "Which columns from the point coordinate file contain the point x, y, (and z) "
            "coordinates? Please enter a line named \"Points\" to the attribute table.");
        return false;
    }

    // Build point set / polygonal data structure based on the input columns and specs.
    // This will create copies of the data in some cases. If not, it would need to invalidate
    // m_coordinateData, which would be impractical in case of an error below.
    // Alternative: extend MatricesToVtk to use arbitrary columns if a io::InputVector for
    // populating VTK objects.

    io::InputVector coords;
    for (const int c : coordsAttribute.columns)
    {
        assert(c >= 1 && size_t(c) <= m_coordinateData.size());
        coords.emplace_back(m_coordinateData[static_cast<size_t>(c - 1)]);
    }
    if (coords.size() == 2)
    {
        coords.emplace_back(coords[0].size(), io::t_FP(0.0f));
    }
    assert(coords.size() == 3);
    assert(coords[0].size() == coords[1].size() && coords[1].size() == coords[2].size());

    vtkSmartPointer<vtkPolyData> polyData;
    if (m_indexData.empty())
    {
        polyData = MatricesToVtk::parsePoints(
            coords, 0,
            m_importDataType);
    }
    else
    {
        polyData = MatricesToVtk::parseIndexedTriangles(
            coords, 0, 1,
            m_indexData, 0,
            m_importDataType);
    }

    if (!polyData)
    {
        QString msg = "The input data is not valid or unsupported.";
        if (!m_indexData.empty())
        {
            msg.append(
                " Please check that all triangle vertex indices refer to valid coordinates.");
        }
        QMessageBox::warning(this, "Import Failed", msg);
        return false;
    }

    {
        CoordinateSystemSpecification coordsSpec;
        coordsSpec.unitOfMeasurement = coordsAttribute.unit;
        coordsSpec.writeToDataSet(*polyData);
    }

    for (const auto & attribute : pointAttributes)
    {
        io::InputVector columnData;
        for (const int c : attribute.columns)
        {
            assert(c >= 1 && size_t(c) <= m_coordinateData.size());
            columnData.emplace_back(m_coordinateData[static_cast<size_t>(c - 1)]);
        }
        auto attributeArray = MatricesToVtk::parseFloatVector(
            columnData, attribute.name,
            0, columnData.size() - 1,
            m_importDataType);
        if (!attributeArray)
        {
            QMessageBox::warning(this, "Import Failed",
                QString("Invalid or unsupported attribute data in attribute %1")
                    .arg(attribute.name));
            return false;
        }
        if (!attribute.unit.isEmpty())
        {
            attributeArray->GetInformation()->Set(
                vtkDataArray::UNITS_LABEL(),
                attribute.unit.toUtf8().data());
        }
        polyData->GetPointData()->AddArray(attributeArray);
    }

    auto baseName = QFileInfo(m_indexData.empty()
        ? m_ui->coordsFileEdit->text()
        : m_ui->indicesFileEdit->text()).baseName();

    bool ok;
    auto name = QInputDialog::getText(this, "Data Object Name", "Data Object Name",
        QLineEdit::Normal, baseName, &ok);
    if (name.isEmpty())
    {
        return false;
    }

    m_polyData = GenericPolyDataObject::createInstance(name, *polyData);

    return true;
}

void DataImporterWidget::updatePointFileAttributesTable()
{
    // Make sure that there is always exactly one empty row at the end.
    // "Past the end", there is one disable line presenting the currently unused columns that are
    // available in the current point coordinates file.

    auto table = m_ui->pointAttributesTable;
    const QSignalBlocker signalBlocker(table);
    auto isRowEmpty = [table] (int row) {
        assert(row < table->rowCount());
        auto col0 = table->item(row, 0);
        auto col1 = table->item(row, 1);
        auto col2 = table->item(row, 2);
        bool val = (!col0 || col0->data(Qt::DisplayRole).toString().isEmpty())
            && (!col1 || col1->data(Qt::DisplayRole).toString().isEmpty())
            && (!col2 || col2->data(Qt::DisplayRole).toString().isEmpty());
        return val;
    };

    assert(m_ui->pointAttributesTable->rowCount() > 1);

    int lastRowWithData = m_ui->pointAttributesTable->rowCount() - 2;
    while (lastRowWithData > 0 && isRowEmpty(lastRowWithData))
    {
        --lastRowWithData;
    }
    const int oldRowCount = table->rowCount();
    const int newRowCount = lastRowWithData + 3;
    table->setRowCount(newRowCount);
    if (newRowCount > oldRowCount)
    {
        table->setItem(oldRowCount - 1, 0, nullptr);
        table->setItem(oldRowCount - 1, 1, nullptr);
        table->setItem(oldRowCount - 1, 2, nullptr);
    }
    table->setItem(newRowCount - 1, 0, new QTableWidgetItem("Unused File Columns"));
    QString unusedColumnsString = "(no file loaded)";
    if (!m_coordinateData.empty())
    {

        std::vector<bool> usedColumns(m_coordinateData.size(), false);
        for (int row = 0; row <= lastRowWithData; ++row)
        {
            auto item = table->item(row, 1);
            std::vector<int> columns;
            if (parseColumns(
                item ? item->data(Qt::DisplayRole).toString() : QString(),
                m_coordinateData.size(),
                columns))
            {
                for (int c : columns)
                {
                    usedColumns[static_cast<size_t>(c) - 1] = true;
                }
            }
        }
        QStringList unusedColumnsStrList;
        for (int c = 0; c < static_cast<int>(usedColumns.size()); ++c)
        {
            if (!usedColumns[c])
            {
                unusedColumnsStrList << QString::number(c + 1);
            }
        }
        if (unusedColumnsStrList.isEmpty())
        {
            unusedColumnsString = "(none)";
        }
        else
        {
            unusedColumnsString = unusedColumnsStrList.join(", ");
        }
    }
    auto unusedInfoHeader = new QTableWidgetItem("Unused File Columns");
    unusedInfoHeader->setFlags({});
    auto unuesdInfoItem = new QTableWidgetItem(unusedColumnsString);
    unuesdInfoItem->setFlags({});
    table->setItem(newRowCount - 1, 0, unusedInfoHeader);
    table->setItem(newRowCount - 1, 1, unuesdInfoItem);
}

void DataImporterWidget::clearData()
{
    io::InputVector().swap(m_coordinateData);
    io::InputVector().swap(m_indexData);
    m_polyData.reset();
}

QString DataImporterWidget::getOpenFileDialog(const QString & title, bool requestPolyData)
{
    auto && filters = io::fileFormatFilters(requestPolyData
        ? io::Category::PolyData
        : io::Category::CSV);

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
