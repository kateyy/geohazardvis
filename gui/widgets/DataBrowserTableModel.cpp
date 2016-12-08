#include "DataBrowserTableModel.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>

#include <QIcon>

#include <vtkDataSet.h>
#include <vtkFloatArray.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/RawVectorData.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/DataProfile2DDataObject.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/utility/DataExtent.h>
#include <core/utility/qthelper.h>
#include <core/utility/vtkvectorhelper.h>


namespace
{

const int s_numBtnColumns = 3;
const int s_colBtnTable = 0;
const int s_colBtnRenderView = 1;
const int s_colBtnDelete = 2;

const int s_colName = s_numBtnColumns + 0;
const int s_colDataSetType = s_numBtnColumns + 1;
const int s_colReferencePoint = s_numBtnColumns + 2;
const int s_colDimensions = s_numBtnColumns + 3;
const int s_colSpatialExtent = s_numBtnColumns + 4;
const int s_colValueRange = s_numBtnColumns + 5;

const int s_numColumns = s_numBtnColumns + 6;

}


DataBrowserTableModel::DataBrowserTableModel(QObject * parent)
    : QAbstractTableModel(parent)
    , m_dataSetHandler{ nullptr }
    , m_numDataObjects{ 0 }
    , m_numAttributeVectors{ 0 }
{
    m_icons.emplace("rendered", QIcon(":/icons/painting.svg"));
    m_icons.emplace("table", QIcon(":/icons/table.svg"));
    m_icons.emplace("delete_red", QIcon(":/icons/delete_red.svg"));
    m_icons.emplace("assign_to_geometry", QIcon(":/icons/file_yellow_paintings_open.svg"));

    QPixmap pixmap(":/icons/painting.svg"); QImage image = pixmap.toImage();
    for (int i = 0; i < pixmap.width(); ++i)
    {
        for (int j = 0; j < pixmap.height(); ++j)
        {
            QRgb pixel = image.pixel(i, j);
            int gray = qGray(pixel);
            image.setPixel(i, j, qRgba(gray, gray, gray, qAlpha(pixel)));
        };
    }
    m_icons.emplace("notRendered", QIcon(QPixmap().fromImage(image)));
}

DataBrowserTableModel::~DataBrowserTableModel() = default;

void DataBrowserTableModel::setDataSetHandler(const DataSetHandler * dataSetHandler)
{
    m_dataSetHandler = dataSetHandler;
}

int DataBrowserTableModel::rowCount(const QModelIndex &/*parent = QModelIndex()*/) const
{
    return m_numDataObjects + m_numAttributeVectors;
}

int DataBrowserTableModel::columnCount(const QModelIndex &/*parent = QModelIndex()*/) const
{
    return s_numColumns;
}

QVariant DataBrowserTableModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
{
    int row = index.row();
    int column = index.column();

    if (row < m_numDataObjects)
    {
        return data_dataObject(row, column, role);
    }

    return data_attributeVector(row - m_numDataObjects, column, role);
}

QVariant DataBrowserTableModel::headerData(int section, Qt::Orientation orientation,
    int role /*= Qt::DisplayRole*/) const
{
    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (orientation != Qt::Orientation::Horizontal)
    {
        return QVariant();
    }

    switch (section)
    {
    case s_colName: return "Name";
    case s_colDataSetType: return "Data Set Type";
    case s_colReferencePoint: return "Reference Point";
    case s_colDimensions: return "Dimensions";
    case s_colSpatialExtent: return "Spatial Extent";
    case s_colValueRange: return "Value Range";
    }        

    return QVariant();
}

DataObject * DataBrowserTableModel::dataObjectAt(int row) const
{
    assert(m_dataSetHandler);

    if (row < m_numDataObjects)
    {
        return m_dataSetHandler->dataSets()[row];
    }

    row -= m_numDataObjects;
    assert(row < m_numAttributeVectors);

    return m_dataSetHandler->rawVectors()[row];
}

DataObject * DataBrowserTableModel::dataObjectAt(const QModelIndex & index) const
{
    return dataObjectAt(index.row());
}

int DataBrowserTableModel::rowForDataObject(DataObject * dataObject) const
{
    assert(m_dataSetHandler);

    if (auto raw = dynamic_cast<RawVectorData *>(dataObject))
    {
        return m_numDataObjects + m_dataSetHandler->rawVectors().indexOf(raw);
    }

    return m_dataSetHandler->dataSets().indexOf(dataObject);
}

QList<DataObject *> DataBrowserTableModel::dataObjects(QModelIndexList indexes)
{
    QList<DataObject *> result;

    for (const QModelIndex & index : indexes)
    {
        if (auto d = dataObjectAt(index.row()))
        {
            result << d;
        }
    }

    return result;
}

QList<DataObject *> DataBrowserTableModel::dataSets(QModelIndexList indexes)
{
    QList<DataObject *> result;

    for (const QModelIndex & index : indexes)
    {
        if (index.row() >= m_numDataObjects)
        {
            continue;
        }

        if (auto d = dataObjectAt(index.row()))
        {
            result << d;
        }
    }

    return result;
}

QList<DataObject *> DataBrowserTableModel::rawVectors(QModelIndexList indexes)
{
    QList<DataObject *> result;

    for (const QModelIndex & index : indexes)
    {
        if (index.row() <= m_numDataObjects)
        {
            continue;
        }

        if (auto d = dataObjectAt(index.row()))
        {
            result << d;
        }
    }

    return result;
}

int DataBrowserTableModel::numButtonColumns()
{
    return s_numBtnColumns;
}

QString DataBrowserTableModel::componentName(int component, int numComponents)
{
    return numComponents <= 3
        ? QString('x' + component)
        : "[" + QString::number(component) + "]";
}

void DataBrowserTableModel::updateDataList(const QList<DataObject *> & visibleObjects)
{
    assert(m_dataSetHandler);

    beginResetModel();

    const QList<DataObject *> & dataSets = m_dataSetHandler->dataSets();
    const QList<RawVectorData *> & rawVectors = m_dataSetHandler->rawVectors();
    m_numDataObjects = dataSets.size();
    m_numAttributeVectors = rawVectors.size();

    m_visibilities.clear();
    disconnectAll(m_dataObjectConnections);

    auto rowUpdateFunc = [this] (int row)
    {
        const auto left = index(row, 0);
        const auto right = index(row, columnCount() - 1);

        emit dataChanged(left, right, { Qt::DisplayRole });
    };

    int currentRow = 0;

    for (const auto & dataObject : dataSets)
    {
        m_visibilities.emplace(dataObject, visibleObjects.contains(dataObject));

        m_dataObjectConnections.emplace_back(
            connect(dataObject, &DataObject::dataChanged, std::bind(rowUpdateFunc, currentRow)));
        m_dataObjectConnections.emplace_back(
            connect(dataObject, &DataObject::boundsChanged, std::bind(rowUpdateFunc, currentRow)));
        m_dataObjectConnections.emplace_back(
            connect(dataObject, &DataObject::valueRangeChanged, std::bind(rowUpdateFunc, currentRow)));
        m_dataObjectConnections.emplace_back(
            connect(dataObject, &DataObject::attributeArraysChanged, std::bind(rowUpdateFunc, currentRow)));
        m_dataObjectConnections.emplace_back(
            connect(dataObject, &DataObject::structureChanged, std::bind(rowUpdateFunc, currentRow)));
        ++currentRow;
    }
    for (const auto & attr : rawVectors)
    {
        m_visibilities.emplace(attr, visibleObjects.contains(attr));

        m_dataObjectConnections.emplace_back(
            connect(attr, &DataObject::dataChanged, std::bind(rowUpdateFunc, currentRow)));
        m_dataObjectConnections.emplace_back(
            connect(attr, &DataObject::boundsChanged, std::bind(rowUpdateFunc, currentRow)));
        m_dataObjectConnections.emplace_back(
            connect(attr, &DataObject::valueRangeChanged, std::bind(rowUpdateFunc, currentRow)));
        m_dataObjectConnections.emplace_back(
            connect(attr, &DataObject::attributeArraysChanged, std::bind(rowUpdateFunc, currentRow)));
        m_dataObjectConnections.emplace_back(
            connect(attr, &DataObject::structureChanged, std::bind(rowUpdateFunc, currentRow)));
        ++currentRow;
    }

    endResetModel();
}

QVariant DataBrowserTableModel::data_dataObject(int row, int column, int role) const
{
    assert(m_dataSetHandler);

    static const auto degreeSign = QString(QChar(0x00B0));

    const QList<DataObject *> & dataSets = m_dataSetHandler->dataSets();
    assert(row < dataSets.size());

    auto dataObject = dataSets.at(row);

    if (role == Qt::DecorationRole)
    {
        switch (column)
        {
        case s_colBtnTable:
            return m_icons.at("table");
        case s_colBtnRenderView:
            return m_visibilities.at(dataObjectAt(row))
                ? m_icons.at("rendered")
                : m_icons.at("notRendered");
        case s_colBtnDelete:
            return m_dataSetHandler->dataSetOwnerships().value(dataObject, false)
                ? QVariant(m_icons.at("delete_red"))
                : QVariant();
        default:
            return{};
        }
    }

    if (role == Qt::ToolTipRole)
    {
        switch (column)
        {
        case s_colReferencePoint:
            if (auto transformable = dynamic_cast<CoordinateTransformableDataObject *>(dataObject))
            {
                auto && coordsSpec = transformable->coordinateSystem();
                if (!coordsSpec.isValid())
                {
                    return{};
                }
                QString toolTip = "Coordinate System: "
                    + coordsSpec.geographicSystem + ", " + coordsSpec.globalMetricSystem
                    + (coordsSpec.type == CoordinateSystemType::metricLocal
                        ? " (Local coordinates)" : "");
                const int maxlRef = 10000;
                const int centerlRef = maxlRef / 2;
                auto && lRef = coordsSpec.referencePointLocalRelative;
                const auto lRefI = convertTo<int>(lRef * double(maxlRef));
                toolTip += "\nReference Point within Data Set: ";
                if (lRefI[0] == centerlRef && lRefI[1] == centerlRef)
                {
                    toolTip += "Center";
                }
                else
                {
                    const auto NRef = QString(lRefI[1] == 0 ? "North" : (lRefI[1] == maxlRef ? "South" : ""));
                    const auto ERef = QString(lRefI[0] == 0 ? "East" : (lRefI[0] == maxlRef ? "West" : ""));
                    if (!NRef.isEmpty() && !ERef.isEmpty())
                    {
                        toolTip += NRef + " " + ERef;
                    }
                    else
                    {
                        toolTip += QString::number(static_cast<int>(lRef[1] * 100.0)) + "% North, "
                            + QString::number(static_cast<int>(lRef[0] * 100.0)) + "% East";
                    }
                }
                return toolTip;
            }
        default:
            return{};
        }

    }

    if (role != Qt::DisplayRole)
    {
        return{};
    }

    auto && dataTypeName = dataObject->dataTypeName();

    switch (column)
    {
    case s_colName:
        return dataObject->name();
    case s_colDataSetType:
        return dataObject->dataTypeName();
    case s_colReferencePoint:
        if (auto transformable = dynamic_cast<CoordinateTransformableDataObject *>(dataObject))
        {
            auto && coordsSpec = transformable->coordinateSystem();
            auto && ref = coordsSpec.referencePointLatLong;
            return coordsSpec.isReferencePointValid()
                ? QString::number(std::abs(ref.GetX())) + degreeSign + (ref.GetX() >= 0 ? "N" : "S") + ", "
                + QString::number(std::abs(ref.GetY())) + degreeSign + (ref.GetY() >= 0 ? "E" : "W")
                : QString();
        }
    case s_colDimensions:
        if (dataTypeName == "Polygonal Mesh")
        {
            return
                QString::number(dataObject->dataSet()->GetNumberOfCells()) + " triangles, " +
                QString::number(dataObject->dataSet()->GetNumberOfPoints()) + " points";
        }
        if (dataTypeName == "Point Cloud")
        {
            return QString::number(dataObject->dataSet()->GetNumberOfPoints()) + " points";
        }
        if (dataTypeName == "Regular 2D Grid")
        {
            auto & imageData = static_cast<ImageDataObject &>(*dataObject);
            auto && dimensions = imageData.extent().componentSize();
            return QString::number(dimensions[0]) + "x" + QString::number(dimensions[1]) + " values";
        }
        if (dataTypeName == "Data Set Profile (2D)")
        {
            return QString::number(static_cast<DataProfile2DDataObject *>(dataObject)->numberOfScalars()) + " values";
        }
        if (dataTypeName == "Regular 3D Grid")
        {
            auto & gridData = static_cast<VectorGrid3DDataObject &>(*dataObject);
            auto && dimensions = gridData.extent().componentSize();
            return QString::number(dimensions[0]) + "x" + QString::number(dimensions[1]) + "x" + QString::number(dimensions[2]) + " vectors";
        }
        break;
    case s_colSpatialExtent:
        return
            "x: " + QString::number(dataObject->bounds()[0]) + "; " + QString::number(dataObject->bounds()[1]) +
            ", y: " + QString::number(dataObject->bounds()[2]) + "; " + QString::number(dataObject->bounds()[3]) +
            ", z: " + QString::number(dataObject->bounds()[4]) + "; " + QString::number(dataObject->bounds()[5]);
    case s_colValueRange:
        if (dataTypeName == "Polygonal Mesh")
        {
            return "";
        }
        if (dataTypeName == "Regular 2D Grid")
        {
            auto && range = static_cast<ImageDataObject *>(dataObject)->scalarRange();
            return QString::number(range[0]) + "; " + QString::number(range[1]);
        }
        if (dataTypeName == "Data Set Profile (2D)")
        {
            auto && range = static_cast<DataProfile2DDataObject *>(dataObject)->scalarRange();
            return QString::number(range[0]) + "; " + QString::number(range[1]);
        }
        if (dataTypeName == "Regular 3D Grid")
        {
            auto vectorGrid = static_cast<VectorGrid3DDataObject*>(dataObject);
            QString line;
            int numComponents = vectorGrid->numberOfComponents();
            for (int c = 0; c < numComponents; ++c)
            {
                line += componentName(c, numComponents) + ": " + QString::number(vectorGrid->scalarRange(c)[0]) + "; " + QString::number(vectorGrid->scalarRange(c)[1]) + ", ";
            }
            line.chop(2);
            return line;
        }
        break;
    }

    return QVariant();
}

QVariant DataBrowserTableModel::data_attributeVector(int row, int column, int role) const
{
    assert(m_dataSetHandler);

    const QList<RawVectorData *> & rawVectors = m_dataSetHandler->rawVectors();
    assert(row < rawVectors.size());

    auto attributeVector = rawVectors.at(row);

    if (role == Qt::DecorationRole)
    {
        switch (column)
        {
        case s_colBtnTable: return m_icons.at("table");
        case s_colBtnRenderView: return m_icons.at("assign_to_geometry");
        case s_colBtnDelete:
            return m_dataSetHandler->rawVectorOwnerships().value(attributeVector, false)
                ? QVariant(m_icons.at("delete_red"))
                : QVariant();
        default: return QVariant();
        }
    }

    if (role == Qt::ToolTipRole)
    {
        switch (column)
        {
        case s_colBtnRenderView: return "Assign to Geometry";
        default: return QVariant();
        }
    }

    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    auto dataArray = attributeVector->dataArray();
    int numComponents = dataArray->GetNumberOfComponents();

    switch (column)
    {
    case s_colName:
        return attributeVector->name();
    case s_colDataSetType:
        return QString::number(numComponents) + " component vector";
    case s_colDimensions:
        return QString::number(dataArray->GetNumberOfTuples()) + " tuples";
    case s_colSpatialExtent:
        return "";
    case s_colValueRange:
    {
        QString ranges;
        std::array<double, 2> range;
        for (int c = 0; c < numComponents; ++c)
        {
            dataArray->GetRange(range.data(), c);
            ranges += componentName(c, numComponents) + ": " + QString::number(range[0]) + "; " + QString::number(range[1]) + ", ";
        }
        ranges.remove(ranges.length() - 2, 2);
        return ranges;
    }
    }

    return QVariant();
}
