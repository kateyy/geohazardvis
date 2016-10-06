#include "DataBrowserTableModel.h"

#include <algorithm>
#include <cassert>
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


namespace
{

const int s_btnClms = 3;

}


DataBrowserTableModel::DataBrowserTableModel(QObject * parent)
    : QAbstractTableModel(parent)
    , m_dataSetHandler{ nullptr }
    , m_numDataObjects{ 0 }
    , m_numAttributeVectors{ 0 }
{
    m_icons.insert("rendered", QIcon(":/icons/painting.svg"));
    m_icons.insert("table", QIcon(":/icons/table.svg"));
    m_icons.insert("delete_red", QIcon(":/icons/delete_red.svg"));
    m_icons.insert("assign_to_geometry", QIcon(":/icons/file_yellow_paintings_open.svg"));

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
    m_icons.insert("notRendered", QIcon(QPixmap().fromImage(image)));
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
    return s_btnClms + 5;
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
    case s_btnClms + 0: return "Name";
    case s_btnClms + 1: return "Data Set Type";
    case s_btnClms + 2: return "Dimensions";
    case s_btnClms + 3: return "Spatial Extent";
    case s_btnClms + 4: return "Value Range";
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
    return s_btnClms;
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

    for (auto dataObject : dataSets)
    {
        m_visibilities.insert(dataObject, visibleObjects.contains(dataObject));

        m_dataObjectConnections <<
            connect(dataObject, &DataObject::dataChanged, std::bind(rowUpdateFunc, currentRow));
        m_dataObjectConnections <<
            connect(dataObject, &DataObject::boundsChanged, std::bind(rowUpdateFunc, currentRow));
        m_dataObjectConnections <<
            connect(dataObject, &DataObject::valueRangeChanged, std::bind(rowUpdateFunc, currentRow));
        m_dataObjectConnections <<
            connect(dataObject, &DataObject::attributeArraysChanged, std::bind(rowUpdateFunc, currentRow));
        m_dataObjectConnections <<
            connect(dataObject, &DataObject::structureChanged, std::bind(rowUpdateFunc, currentRow));
        ++currentRow;
    }
    for (auto attr : rawVectors)
    {
        m_visibilities.insert(attr, visibleObjects.contains(attr));

        m_dataObjectConnections <<
            connect(attr, &DataObject::dataChanged, std::bind(rowUpdateFunc, currentRow));
        m_dataObjectConnections <<
            connect(attr, &DataObject::boundsChanged, std::bind(rowUpdateFunc, currentRow));
        m_dataObjectConnections <<
            connect(attr, &DataObject::valueRangeChanged, std::bind(rowUpdateFunc, currentRow));
        m_dataObjectConnections <<
            connect(attr, &DataObject::attributeArraysChanged, std::bind(rowUpdateFunc, currentRow));
        m_dataObjectConnections <<
            connect(attr, &DataObject::structureChanged, std::bind(rowUpdateFunc, currentRow));
        ++currentRow;
    }

    endResetModel();
}

QVariant DataBrowserTableModel::data_dataObject(int row, int column, int role) const
{
    assert(m_dataSetHandler);

    const QList<DataObject *> & dataSets = m_dataSetHandler->dataSets();
    assert(row < dataSets.size());

    auto dataObject = dataSets.at(row);

    if (role == Qt::DecorationRole)
    {
        switch (column)
        {
        case 0:
            return m_icons["table"];
        case 1:
            return m_visibilities[dataObjectAt(row)]
                ? m_icons["rendered"]
                : m_icons["notRendered"];
        case 2:
            return m_dataSetHandler->dataSetOwnerships().value(dataObject, false)
                ? QVariant(m_icons["delete_red"])
                : QVariant();
        }
    }

    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    QString dataTypeName = dataObject->dataTypeName();

    switch (column)
    {
    case s_btnClms:
        return dataObject->name();
    case s_btnClms + 1:
        return dataObject->dataTypeName();
    case s_btnClms + 2:
        if (dataTypeName == "polygonal mesh")
        {
            return
                QString::number(dataObject->dataSet()->GetNumberOfCells()) + " triangles, " +
                QString::number(dataObject->dataSet()->GetNumberOfPoints()) + " vertices";
        }
        if (dataTypeName == "regular 2D grid")
        {
            auto & imageData = static_cast<ImageDataObject &>(*dataObject);
            auto && dimensions = imageData.extent().componentSize();
            return QString::number(dimensions[0]) + "x" + QString::number(dimensions[1]) + " values";
        }
        if (dataTypeName == "Data Set Profile (2D)")
        {
            return QString::number(static_cast<DataProfile2DDataObject *>(dataObject)->numberOfScalars()) + " values";
        }
        if (dataTypeName == "3D vector grid")
        {
            auto & gridData = static_cast<VectorGrid3DDataObject &>(*dataObject);
            auto && dimensions = gridData.extent().componentSize();
            return QString::number(dimensions[0]) + "x" + QString::number(dimensions[1]) + "x" + QString::number(dimensions[2]) + " vectors";
        }
        break;
    case s_btnClms + 3:
        return
            "x: " + QString::number(dataObject->bounds()[0]) + "; " + QString::number(dataObject->bounds()[1]) +
            ", y: " + QString::number(dataObject->bounds()[2]) + "; " + QString::number(dataObject->bounds()[3]) +
            ", z: " + QString::number(dataObject->bounds()[4]) + "; " + QString::number(dataObject->bounds()[5]);
    case s_btnClms + 4:
        if (dataTypeName == "polygonal mesh")
        {
            return "";
        }
        if (dataTypeName == "regular 2D grid")
        {
            auto && range = static_cast<ImageDataObject *>(dataObject)->scalarRange();
            return QString::number(range[0]) + "; " + QString::number(range[1]);
        }
        if (dataTypeName == "Data Set Profile (2D)")
        {
            auto && range = static_cast<DataProfile2DDataObject *>(dataObject)->scalarRange();
            return QString::number(range[0]) + "; " + QString::number(range[1]);
        }
        if (dataTypeName == "3D vector grid")
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
        case 0: return m_icons["table"];
        case 1: return m_icons["assign_to_geometry"];
        case 2:
            return m_dataSetHandler->rawVectorOwnerships().value(attributeVector, false)
                ? QVariant(m_icons["delete_red"])
                : QVariant();
        default: return QVariant();
        }
    }

    if (role == Qt::ToolTipRole)
    {
        switch (column)
        {
        case 1: return "Assign to Geometry";
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
    case s_btnClms:
        return attributeVector->name();
    case s_btnClms + 1:
        return QString::number(numComponents) + " component vector";
    case s_btnClms + 2:
        return QString::number(dataArray->GetNumberOfTuples()) + " tuples";
    case s_btnClms + 3:
        return "";
    case s_btnClms + 4:
    {
        QString ranges;
        double range[2];
        for (int c = 0; c < numComponents; ++c)
        {
            dataArray->GetRange(range, c);
            ranges += componentName(c, numComponents) + ": " + QString::number(range[0]) + "; " + QString::number(range[1]) + ", ";
        }
        ranges.remove(ranges.length() - 2, 2);
        return ranges;
    }
    }

    return QVariant();
}
