#include "DataBrowserTableModel.h"

#include <cassert>

#include <vtkDataSet.h>

#include <core/data_objects/DataObject.h>
#include <core/data_objects/ImageDataObject.h>


namespace
{

const int s_btnClms = 3;

}


DataBrowserTableModel::DataBrowserTableModel(QObject * parent)
    : QAbstractTableModel(parent)
    , m_rendererFocused(false)
{
    m_icons.insert("rendered", QIcon(":/icons/painting.svg"));
    m_icons.insert("notRendered", QIcon(":/icons/painting_faded.svg"));
    m_icons.insert("table", QIcon(":/icons/table.svg"));
    m_icons.insert("delete_red", QIcon(":/icons/delete_red.svg"));

    {
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
        m_icons.insert("noRenderer", QIcon(QPixmap().fromImage(image)));
    }
}

int DataBrowserTableModel::rowCount(const QModelIndex &/*parent = QModelIndex()*/) const
{
    return m_dataObjects.size();
}

int DataBrowserTableModel::columnCount(const QModelIndex &/*parent = QModelIndex()*/) const
{
    return s_btnClms + 4;
}

QVariant DataBrowserTableModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
{
    if (role == Qt::DecorationRole)
    {
        switch (index.column())
        {
        case 0:
            return QVariant(m_icons["table"]);
        case 1:
            if (!m_rendererFocused)
                return QVariant(m_icons["noRenderer"]);
            if (m_visibilities[dataObjectAt(index)])
                return QVariant(m_icons["rendered"]);
            return QVariant(m_icons["notRendered"]);
        case 2:
            return QVariant(m_icons["delete_red"]);
        }
    }

    if (role != Qt::DisplayRole)
        return QVariant();

    assert(index.row() < m_dataObjects.size());

    DataObject * dataObject = m_dataObjects[index.row()];

    switch (index.column())
    {
    case s_btnClms:
        return QVariant(dataObject->name());
    case s_btnClms + 1:
        return QVariant(dataObject->dataTypeName());
    case s_btnClms + 2:
        if (dataObject->dataTypeName() == "polygonal mesh")
            return QVariant(
                QString::number(dataObject->dataSet()->GetNumberOfCells()) + " triangles; " +
                QString::number(dataObject->dataSet()->GetNumberOfPoints()) + " vertices");
        if (dataObject->dataTypeName() == "regular 2D grid")
        {
            ImageDataObject * imageData = static_cast<ImageDataObject*>(dataObject);
            return QVariant(QString::number(imageData->dimensions()[0]) + "x" + QString::number(imageData->dimensions()[1]) + " values");
        }
    case s_btnClms + 3:
        if (dataObject->dataTypeName() == "polygonal mesh")
            return QVariant(
            "x: " + QString::number(dataObject->bounds()[0]) + "; " + QString::number(dataObject->bounds()[1]) +
            ", y: " + QString::number(dataObject->bounds()[2]) + "; " + QString::number(dataObject->bounds()[3]) +
            ", z: " + QString::number(dataObject->bounds()[4]) + "; " + QString::number(dataObject->bounds()[5]));
        if (dataObject->dataTypeName() == "regular 2D grid")
        {
            const double * minMax = static_cast<ImageDataObject*>(dataObject)->minMaxValue();
            return QString::number(minMax[0]) + "; " + QString::number(minMax[1]);
        }
        
    }

    return QVariant();
}

QVariant DataBrowserTableModel::headerData(int section, Qt::Orientation orientation,
    int role /*= Qt::DisplayRole*/) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    assert(orientation == Qt::Orientation::Horizontal);

    switch (section)
    {
    case s_btnClms + 0: return QVariant("name");
    case s_btnClms + 1: return QVariant("data set type");
    case s_btnClms + 2: return QVariant("dimensions");
    case s_btnClms + 3: return QVariant("value range");
    }        

    return QVariant();
}

DataObject * DataBrowserTableModel::dataObjectAt(int row) const
{
    if (row < 0 || row >= m_dataObjects.size())
    {
        assert(false);
        return nullptr;
    }

    return m_dataObjects.at(row);
}

DataObject * DataBrowserTableModel::dataObjectAt(const QModelIndex & index) const
{
    return dataObjectAt(index.row());
}

int DataBrowserTableModel::numButtonColumns()
{
    return s_btnClms;
}

void DataBrowserTableModel::addDataObject(DataObject * dataObject)
{
    beginInsertRows(QModelIndex(), m_dataObjects.size(), m_dataObjects.size());
    m_dataObjects << dataObject;
    m_visibilities.insert(dataObject, false);
    endInsertRows();
}

void DataBrowserTableModel::removeDataObject(DataObject * dataObject)
{
    int index = m_dataObjects.indexOf(dataObject);
    beginRemoveRows(QModelIndex(), index, index);
    m_dataObjects.removeAt(index);
    endRemoveRows();
}

void DataBrowserTableModel::setVisibility(const DataObject * dataObject, bool visible)
{
    m_visibilities[dataObject] = visible;
    m_rendererFocused = true;
}

void DataBrowserTableModel::setNoRendererFocused()
{
    m_rendererFocused = false;
}
