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

#include "CSVExporterWidget.h"
#include "ui_CSVExporterWidget.h"

#include <array>
#include <cassert>
#include <functional>

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include <vtkDataArrayAccessor.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkExecutive.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>

#include <core/data_objects/DataProfile2DDataObject.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/io/io_helper.h>
#include <core/io/TextFileWriter.h>
#include <core/utility/types_utils.h>


namespace
{

const std::vector<std::pair<QString, QString>> & delimiters()
{
    static const std::vector<std::pair<QString, QString>> delim = {
        { "Semicolon (;)", ";" },
        { "Comma (,)", "," },
        { "Tab (\t)", "\t" },
        { "Space (    )", "    " }
    };
    return delim;
}
size_t defaultDelimiter()
{
    return 3u;
}

const std::vector<std::pair<QString, QString>> & lineEndings()
{
    static const std::vector<std::pair<QString, QString>> les = {
        { "Unix (LF)", "\n" },
        { "Windows (CR LF)", "\r\n" },
        { "Legacy Macintosh (CR)", "\r" }
    };
    return les;
}
size_t defaultLineEnding()
{
#if _WIN32
    return 1u;
#else
    return 0u;
#endif
}

vtkSmartPointer<vtkIdTypeArray> getTrianglePointIds(PolyDataObject & polyDataObject)
{
    auto poly = vtkPolyData::SafeDownCast(polyDataObject.processedOutputDataSet());
    if (!poly)
    {
        return{};
    }
    auto polys = poly->GetPolys();
    if (!polys)
    {
        return{};
    }
    const vtkIdType numCells = polys->GetNumberOfCells();
    auto triangleIds = vtkSmartPointer<vtkIdTypeArray>::New();
    triangleIds->SetNumberOfComponents(3);
    triangleIds->SetNumberOfTuples(numCells);
    vtkDataArrayAccessor<vtkIdTypeArray> ids{ triangleIds };
    polys->InitTraversal();
    for (vtkIdType i = 0; i < numCells; ++i)
    {
        vtkIdType numCellPoints;
        vtkIdType * cellPointIds = nullptr;
        if (!polys->GetNextCell(numCellPoints, cellPointIds)
            || numCellPoints != 3 || !cellPointIds)
        {
            return {};
        }
        ids.Set(i, cellPointIds);
    }
    return triangleIds;
};

vtkSmartPointer<vtkAbstractArray> getPointCoords(vtkDataSet & dataSet)
{
    if (auto pointSet = vtkPointSet::SafeDownCast(&dataSet))
    {
        return pointSet->GetPoints()->GetData();
    }

    auto points = vtkSmartPointer<vtkDoubleArray>::New();
    points->SetName("Points");
    points->SetNumberOfComponents(3);
    const vtkIdType numPoints = dataSet.GetNumberOfPoints();
    points->SetNumberOfTuples(numPoints);
    std::array<double, 3> point;
    for (vtkIdType i = 0; i < numPoints; ++i)
    {
        dataSet.GetPoint(i, point.data());
        points->SetTuple(i, point.data());
    }
    return points;
}

vtkSmartPointer<vtkAbstractArray> getPlotPosition(vtkDataSet & dataSet)
{
    auto pointSet = vtkPointSet::SafeDownCast(&dataSet);
    if (!pointSet)
    {
        return{};
    }
    auto points = pointSet->GetPoints()->GetData();
    auto position = vtkSmartPointer<vtkDataArray>::Take(points->NewInstance());
    position->SetName("Position");
    position->GetInformation()->CopyEntry(points->GetInformation(), vtkDataArray::UNITS_LABEL());
    position->SetNumberOfComponents(1);
    position->SetNumberOfTuples(points->GetNumberOfTuples());
    position->CopyComponent(0, points, 0);
    return position;
}

}


CSVExporterWidget::CSVExporterWidget(QWidget * parent, Qt::WindowFlags f)
    : QDialog(parent, f)
    , m_ui{ std::make_unique<Ui_CSVExporterWidget>() }
    , m_allCanceled{ false }
    , m_selectedGeometry{ GeometryType::invalid }
{
    m_ui->setupUi(this);
    m_ui->cancelAllButton->hide();
    connect(m_ui->cancelAllButton, &QAbstractButton::clicked, [this] () {
        m_allCanceled = true;
        reject();
    });

    for (const auto & delim : delimiters())
    {
        m_ui->delimiterCombo->addItem(delim.first);
    }
    m_ui->delimiterCombo->setCurrentIndex(static_cast<int>(defaultDelimiter()));
    for (const auto & le : lineEndings())
    {
        m_ui->lineEndingCombo->addItem(le.first);
    }
    m_ui->lineEndingCombo->setCurrentIndex(static_cast<int>(defaultLineEnding()));

    for (auto radio : m_ui->geometryGroupBox->findChildren<QRadioButton *>())
    {
        connect(radio, &QAbstractButton::toggled,
            this, &CSVExporterWidget::updateAttributesForGeometrySelection);
    }

    clear();

    connect(m_ui->exportButton, &QAbstractButton::clicked, this, &CSVExporterWidget::exportData);
}

CSVExporterWidget::~CSVExporterWidget() = default;

void CSVExporterWidget::setDataObject(DataObject * dataObject)
{
    clear();

    m_dataObject = dataObject;
    m_profileDataObject = dynamic_cast<DataProfile2DDataObject *>(dataObject);
    m_imageDataObject = dynamic_cast<ImageDataObject *>(dataObject);
    m_polyDataObject = dynamic_cast<PolyDataObject *>(dataObject);

    const QSignalBlocker scalarGridBlocker{ m_ui->geomScalarGrid2D };
    const QSignalBlocker pointsBlocker{ m_ui->geomPointsRadioButton };
    const QSignalBlocker trianglesBlocker{ m_ui->geomTrianglesRadioButton };

    m_ui->geomScalarGrid2D->hide();
    m_ui->geomPointsRadioButton->hide();
    m_ui->geomTrianglesRadioButton->hide();

    if (m_dataObject)
    {
        m_ui->dataSetNameLabel->setText(dataObject->name());
        m_ui->geomPointsRadioButton->show();

        if (m_imageDataObject)
        {
            m_selectedGeometry = GeometryType::scalarGrid2D;
            m_ui->geomScalarGrid2D->show();
            m_ui->geomScalarGrid2D->setChecked(true);
        }
        else if (m_polyDataObject)
        {
            m_selectedGeometry = GeometryType::triangles;
            m_ui->geomTrianglesRadioButton->show();
            m_ui->geomTrianglesRadioButton->setChecked(true);
        }
        else
        {
            m_selectedGeometry = GeometryType::points;
            m_ui->geomPointsRadioButton->setChecked(true);
        }
    }

    updateAttributes();
}

DataObject * CSVExporterWidget::dataObject()
{
    return m_dataObject;
}

void CSVExporterWidget::clear()
{
    m_allCanceled = false;
    m_ui->dataSetNameLabel->setText("[none]");
    m_ui->geomPointsRadioButton->hide();
    m_ui->geomPointsRadioButton->setChecked(false);
    m_ui->geomTrianglesRadioButton->hide();
    m_ui->geomTrianglesRadioButton->setChecked(false);
    m_ui->attributesTable->setColumnCount(1);
    m_ui->attributesTable->setRowCount(0);
}

void CSVExporterWidget::setDir(const QString & dir)
{
    m_dir = dir;
}

const QString & CSVExporterWidget::dir() const
{
    return m_dir;
}

void CSVExporterWidget::exportData()
{
    if (!m_dataObject || m_attributes.empty())
    {
        QMessageBox::information(this, windowTitle(), "There is no exportable data!");
        return;
    }

    const int rowCount = m_ui->attributesTable->rowCount();
    assert(m_ui->attributesTable->columnCount() == 1
        && rowCount == static_cast<int>(m_attributes.size()));

    std::vector<vtkSmartPointer<vtkAbstractArray>> attributesToExport;
    std::vector<QString> attributeNames;

    if (m_selectedGeometry == GeometryType::scalarGrid2D)
    {
        int row = 0;
        for (; row < rowCount; ++row)
        {
            auto radioBtn = dynamic_cast<QRadioButton *>(m_ui->attributesTable->cellWidget(row, 0));
            assert(radioBtn);
            if (radioBtn && radioBtn->isChecked())
            {
                break;
            }
        }
        if (row == rowCount)
        {
            QMessageBox::information(this, windowTitle(),
                "Please select exactly one attribute to export in the attribute table!");
            return;
        }
        auto & attr = m_attributes[static_cast<size_t>(row)];
        attributesToExport.push_back(attr.dataExtractor());
        attributeNames.push_back(attr.label);
    }
    else
    {
        for (int row = 0; row < rowCount; ++row)
        {
            if (m_ui->attributesTable->item(row, 0)->checkState() != Qt::Checked)
            {
                continue;
            }
            auto & attr = m_attributes[static_cast<size_t>(row)];
            attributesToExport.push_back(attr.dataExtractor());
            attributeNames.push_back(attr.label);
        }

        if (attributesToExport.empty())
        {
            QMessageBox::information(this, windowTitle(),
                "Please select at least one attribute to export in the attribute table!");
            return;
        }
    }

    const vtkIdType numTuples = attributesToExport.front()->GetNumberOfTuples();
    for (size_t i = 0; i < attributesToExport.size(); ++i)
    {
        const auto & array = attributesToExport[i];
        if (!array)
        {
            QMessageBox::warning(this, windowTitle(),
                QString("Export failed: Could not retrieve data for attribute \"%1\".")
                .arg(m_attributes[i].label));
            return;
        }
        if (array->GetNumberOfTuples() != numTuples)
        {
            const auto firstName = m_attributes.front().label;
            const auto currentName = m_attributes[i].label;
            QMessageBox::warning(this, windowTitle(),
                QString("Export failed: All attribute need to have the same number of tuples. "
                    "(\"%1\" has %2 tuples, \"%3\" has %4 tuples)")
                .arg(firstName).arg(numTuples).arg(currentName).arg(array->GetNumberOfTuples()));
            return;
        }
    }

    const auto delimiter = delimiterFromUi();
    QString ext;
    if (delimiter == ";" || delimiter == ",")
    {
        ext = ".csv";
    }
    else if (delimiter == "\t")
    {
        ext = ".tsv";
    }
    else
    {
        ext = ".txt";
    }
    
    auto suggestedFN = io::normalizeFileName(m_dataObject->name());
    if (m_polyDataObject)
    {
        if (m_selectedGeometry == GeometryType::points)
        {
            suggestedFN += " (Point Coordinates)";
        }
        else if (m_selectedGeometry == GeometryType::triangles)
        {
            suggestedFN += " (Triangle Vertices)";
        }
    }
    suggestedFN = QDir(m_dir).filePath(suggestedFN + ext);

    const auto fileName = QFileDialog::getSaveFileName(this, "Export As", suggestedFN,
        io::fileFormatFilters(io::Category::CSV));
    if (fileName.isEmpty())
    {
        return;
    }
    m_dir = QFileInfo(fileName).path();

    TextFileWriter writer;
    writer.setOutputFileName(fileName);
    writer.setDelimiter(delimiter);
    writer.setStringDelimiter(m_ui->stringDelimiterEdit->text());
    writer.setLineEnding(lineEndingFromUi());

    bool success = true;

    if (m_ui->printHeadersCheckBox->isChecked())
    {
        int columnCount = 0;
        for (const auto & array : attributesToExport)
        {
            columnCount += array->GetNumberOfComponents();
        }
        auto header = vtkSmartPointer<vtkStringArray>::New();
        header->SetNumberOfComponents(columnCount);
        header->SetNumberOfTuples(1);
        int currentColumn = 0;
        for (size_t attributeIndex = 0; attributeIndex < attributesToExport.size(); ++attributeIndex)
        {
            const auto & array = attributesToExport[attributeIndex];
            const auto & attributeName = attributeNames[attributeIndex];
            if (array->GetNumberOfComponents() == 1)
            {
                header->SetValue(currentColumn++, attributeName.toUtf8().data());
                continue;
            }
            for (int c = 0; c < array->GetNumberOfComponents(); ++c)
            {
                header->SetValue(currentColumn++,
                    (attributeName + ":" + QString::number(c)).toUtf8().data());
            }
        }
        assert(currentColumn == columnCount);
        success = success && writer.write(*header);
    }

    success = success && writer.write(attributesToExport);

    if (!success)
    {
        QMessageBox::critical(this, "Export Failed",
            "An error occurred during export. Is the target file writable?");
        return;
    }

    if (!m_ui->keepOpenCheckBox->isChecked())
    {
        accept();
    }
}

void CSVExporterWidget::setShowCancelAllButton(bool showCancelAll)
{
    m_ui->cancelAllButton->setVisible(showCancelAll);
}

bool CSVExporterWidget::showCancelAllButton() const
{
    return m_ui->cancelAllButton->isVisible();
}

bool CSVExporterWidget::allCanceled() const
{
    return m_allCanceled;
}

void CSVExporterWidget::updateAttributesForGeometrySelection(bool checked)
{
    // Ignore radio button updates if it is not the chosen one.
    if (!checked)
    {
        return;
    }

    GeometryType newGeometry = GeometryType::invalid;
    if (sender() == m_ui->geomScalarGrid2D)
    {
        newGeometry = GeometryType::scalarGrid2D;
    }
    else if (sender() == m_ui->geomPointsRadioButton)
    {
        newGeometry = GeometryType::points;
    }
    else if (sender() == m_ui->geomTrianglesRadioButton)
    {
        newGeometry = GeometryType::triangles;
    }

    if (newGeometry == m_selectedGeometry)
    {
        return;
    }

    m_selectedGeometry = newGeometry;
    updateAttributes();
}

void CSVExporterWidget::updateAttributes()
{
    m_attributes.clear();

    if (!m_dataObject)
    {
        return;
    }

    /**
     * Construct a list of named attributes and functions to fetch attribute data on demand.
     * Getter functions are wrapped into checker or polyChecker: Before retrieving an attribute,
     * they will check if the m_dataObject still exists and defer signals/updates it might trigger.
    */

    auto checkerDS = [this] (std::function<vtkSmartPointer<vtkAbstractArray>(vtkDataSet &)> func)
        ->vtkSmartPointer<vtkAbstractArray>
    {
        if (!m_dataObject)
        {
            return{};
        }
        const ScopedEventDeferral deferral{ m_dataObject };
        if (auto ds = m_dataObject->processedOutputDataSet())
        {
            return func(*ds);
        }
        return{};
    };
    auto polyChecker =
        [this](std::function<vtkSmartPointer<vtkAbstractArray>(PolyDataObject &)> func)
        -> vtkSmartPointer<vtkAbstractArray>
    {
        if (!m_polyDataObject)
        {
            return nullptr;
        }
        const ScopedEventDeferral deferral{ m_polyDataObject };
        return func(*m_polyDataObject);
    };


    auto addAttributeArrays = [this, checkerDS] ()
    {
        // Extract all useful attributes from the selected part of the geometry (points/cells).
        const auto attributeUtil = IndexType_util(geometryToIndexType(m_selectedGeometry));
        if (auto dsAttributes = attributeUtil.extractAttributes(m_dataObject->processedOutputDataSet()))
        {
            const int numArrays = dsAttributes->GetNumberOfArrays();
            for (int i = 0; i < numArrays; ++i)
            {
                auto array = dsAttributes->GetAbstractArray(i);
                if (!array || !array->GetName())
                {
                    continue;
                }
                if (array->GetInformation()->Has(DataObject::ARRAY_IS_AUXILIARY()))
                {
                    continue;
                }
                const auto name = QString::fromUtf8(array->GetName());
                auto attributeGetter = [attributeUtil, name] (vtkDataSet & dataSet)
                {
                    return attributeUtil.extractAbstractArray(dataSet, name);
                };
                m_attributes.push_back({ name, false, std::bind(checkerDS, attributeGetter) });
            }
        }
    };

    auto addAttributeArraysOnGrid2D = [this, checkerDS] ()
    {
        assert(m_selectedGeometry == GeometryType::scalarGrid2D);
        // Extract all useful attributes that are mapped to the regular point grid.
        const auto attributeUtil = IndexType_util(IndexType::points);
        if (auto dsAttributes = attributeUtil.extractAttributes(m_dataObject->processedOutputDataSet()))
        {
            const int numArrays = dsAttributes->GetNumberOfArrays();
            for (int i = 0; i < numArrays; ++i)
            {
                auto array = dsAttributes->GetArray(i);
                if (!array || !array->GetName())
                {
                    continue;
                }
                if (array->GetInformation()->Has(DataObject::ARRAY_IS_AUXILIARY()))
                {
                    continue;
                }
                if (array->GetNumberOfComponents() != 1)
                {
                    continue;
                }
                const auto name = QString::fromUtf8(array->GetName());
                auto attributeGetter = [attributeUtil, name] (vtkDataSet & dataSet)
                    -> vtkSmartPointer<vtkAbstractArray>
                {
                    auto image = vtkImageData::SafeDownCast(&dataSet);
                    if (!image)
                    {
                        return{};
                    }
                    auto baseArray = attributeUtil.extractArray(dataSet, name);
                    if (!baseArray)
                    {
                        return{};
                    }
                    std::array<int, 3> dimensions;
                    image->GetDimensions(dimensions.data());
                    auto targetArray = vtkSmartPointer<vtkDataArray>::Take(baseArray->NewInstance());
                    targetArray->SetNumberOfComponents(dimensions[0]);  // X
                    targetArray->SetNumberOfTuples(dimensions[1]);      // Y
                    // Assuming continuous x values here!
                    const auto sourceSize = baseArray->GetDataSize() * baseArray->GetDataTypeSize();
                    const auto targetSize = targetArray->GetDataSize() * targetArray->GetDataTypeSize();
                    if (sourceSize == 0 || sourceSize != targetSize)
                    {
                        return{};
                    }
                    auto sourceData = static_cast<const uint8_t *>(baseArray->GetVoidPointer(0));
                    auto targetData = static_cast<uint8_t *>(targetArray->GetVoidPointer(0));
                    if (!sourceData || !targetData)
                    {
                        return{};
                    }
                    std::copy_n(sourceData, sourceSize, targetData);
                    targetArray->Modified();
                    return targetArray;
                };
                m_attributes.push_back({ name, false, std::bind(checkerDS, attributeGetter) });
            }
        }
    };

    auto updateAttributesUI = [this] (const bool exclusiveSelection = false)
    {
        // Sync attributes into the table widget.
        const int rowCount = static_cast<int>(m_attributes.size());
        m_ui->attributesTable->clearContents();
        m_ui->attributesTable->setRowCount(rowCount);
        if (rowCount == 0)
        {
            return;
        }

        if (!exclusiveSelection)
        {
            const Qt::ItemFlags itemFlags = Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
            for (int row = 0; row < rowCount; ++row)
            {
                const auto & attr = m_attributes[static_cast<size_t>(row)];
                auto item = std::make_unique<QTableWidgetItem>(attr.label);
                item->setFlags(itemFlags);
                item->setCheckState(attr.selectedByDefault ? Qt::Checked : Qt::Unchecked);
                m_ui->attributesTable->setItem(row, 0, item.release());
            }
        }
        else
        {
            bool haveSelection = false;
            for (int row = 0; row < rowCount; ++row)
            {
                const auto & attr = m_attributes[static_cast<size_t>(row)];
                auto radioButton = std::make_unique<QRadioButton>(attr.label);
                const bool doSelect = attr.selectedByDefault && !haveSelection;
                radioButton->setChecked(doSelect);
                m_ui->attributesTable->setCellWidget(row, 0, radioButton.release());
            }
            if (!haveSelection)
            {
                static_cast<QRadioButton *>(m_ui->attributesTable->cellWidget(0, 0))->setChecked(true);
            }
        }
    };

    // Special case: scalar grid can only contain exactly one scalar array (2D).
    if (m_selectedGeometry == GeometryType::scalarGrid2D)
    {
        addAttributeArraysOnGrid2D();
        updateAttributesUI(true);
        return;
    }

    // Special case: profile plots: Pass 1D position array and plot scalars
    if (m_profileDataObject)
    {
        m_attributes.push_back({ "Position", true, std::bind(checkerDS, getPlotPosition) });
        addAttributeArrays();
        for (auto & attr : m_attributes)
        {
            attr.selectedByDefault = true;
        }
        updateAttributesUI();
        return;
    }

    // Indices for all kinds of data sets: users may want to have an index column in the beginning.
    QString indicesName;
    bool includeIndicesByDefault = false;
    if (m_selectedGeometry == GeometryType::points)
    {
        indicesName = "Point Index";
        includeIndicesByDefault = true;
    }
    else if (m_selectedGeometry == GeometryType::triangles && m_polyDataObject)
    {
        indicesName = "Triangle Index";
    }
    else
    {
        indicesName = "Index";
    }
    auto generateIndices = [this, indicesName] (vtkDataSet & dataSet) -> vtkSmartPointer<vtkIdTypeArray>
    {
        vtkIdType numIndices = 0;
        switch (m_selectedGeometry)
        {
        case GeometryType::points:
            numIndices = dataSet.GetNumberOfPoints();
            break;
        case GeometryType::triangles:
            numIndices = dataSet.GetNumberOfCells();
            break;
        default:
            return{};
        }
        auto indices = vtkSmartPointer<vtkIdTypeArray>::New();
        indices->SetNumberOfValues(numIndices);
        indices->SetName(indicesName.toUtf8().data());
        for (vtkIdType i = 0; i < numIndices; ++i)
        {
            indices->SetValue(i, i);
        }
        return indices;
    };
    m_attributes.push_back({ indicesName, includeIndicesByDefault, std::bind(checkerDS, generateIndices) });

    // Triangular geometry: point indices that each triangle is build of.
    if (m_selectedGeometry == GeometryType::triangles && m_polyDataObject)
    {
        m_attributes.push_back({ "Point Indices", true, std::bind(polyChecker, getTrianglePointIds) });
    }

    // When exporting points: point coordinates
    if (m_selectedGeometry == GeometryType::points)
    {
        m_attributes.push_back({ "Point Coordinates", true, std::bind(checkerDS, getPointCoords) });
    }
    // Similarly: centroids as singular representation of triangles.
    else if (m_selectedGeometry == GeometryType::triangles && m_polyDataObject)
    {
        auto getCentroids = [] (PolyDataObject & poly) -> vtkSmartPointer<vtkAbstractArray>
        {
            if (auto cellCenters = poly.cellCenters())
            {
                if (auto points = cellCenters->GetPoints())
                {
                    return points->GetData();
                }
            }
            return{};
        };
        m_attributes.push_back({ "Centroids", true, std::bind(polyChecker, getCentroids) });
    }

    addAttributeArrays();

    updateAttributesUI();
}

IndexType CSVExporterWidget::geometryToIndexType(const GeometryType geometryType)
{
    switch (geometryType)
    {
    case GeometryType::points: return IndexType::points;
    case GeometryType::triangles: return IndexType::cells;
    case GeometryType::scalarGrid2D: return IndexType::points;
    default:
        return IndexType::invalid;
    }
}

QString CSVExporterWidget::delimiterFromUi() const
{
    const auto uiString = m_ui->delimiterCombo->currentText();
    const auto it = std::find_if(delimiters().begin(), delimiters().end(),
        [&uiString] (const std::pair<QString, QString> & delim)
        { return delim.first == uiString || delim.second == uiString; });
    if (it == delimiters().end())
    {
        return uiString;
    }
    return it->second;
}

QString CSVExporterWidget::lineEndingFromUi() const
{
    return lineEndings().at(m_ui->lineEndingCombo->currentIndex()).second;
}
