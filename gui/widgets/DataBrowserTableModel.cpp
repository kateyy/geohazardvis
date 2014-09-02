#include "DataBrowserTableModel.h"

#include <cassert>

#include <vtkDataSet.h>

#include <core/Input.h>
#include <core/data_objects/DataObject.h>


namespace
{

const int s_btnClms = 3;

}


DataBrowserTableModel::DataBrowserTableModel(QObject * parent)
    : QAbstractTableModel(parent)
{
    m_icons.insert("notRendered", QIcon(":/icons/painting.svg"));
    m_icons.insert("rendered", QIcon(":/icons/painting_faded.svg"));
    m_icons.insert("table", QIcon(":/icons/table.svg"));
    m_icons.insert("delete_red", QIcon(":/icons/delete_red.svg"));
}

int DataBrowserTableModel::rowCount(const QModelIndex &/*parent = QModelIndex()*/) const
{
    return m_dataObjects.size();
}

int DataBrowserTableModel::columnCount(const QModelIndex &/*parent = QModelIndex()*/) const
{
    return s_btnClms + 6;
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
        return QVariant(QString::fromStdString(dataObject->input()->name));
    case s_btnClms + 1:
        return QVariant(dataObject->dataTypeName());
    case s_btnClms + 2:
        switch (dataObject->input()->type)
        {
        case ModelType::triangles:
            return QVariant(
                QString::number(dataObject->input()->data()->GetNumberOfCells()) + " triangles; " +
                QString::number(dataObject->input()->data()->GetNumberOfPoints()) + " vertices");
        case ModelType::grid2d:
            GridDataInput * grid = static_cast<GridDataInput*>(dataObject->input().get());
            return QVariant(QString::number(grid->dimensions()[0]) + "x" + QString::number(grid->dimensions()[0]) + " values");
        }
    case s_btnClms + 3:
        return QVariant(
            QString::number(dataObject->input()->bounds()[0]) + "; " +
            QString::number(dataObject->input()->bounds()[1]));
    case s_btnClms + 4:
        return QVariant(
            QString::number(dataObject->input()->bounds()[2]) + "; " +
            QString::number(dataObject->input()->bounds()[3]));
    case s_btnClms + 5:
        return QVariant(
            QString::number(dataObject->input()->bounds()[4]) + "; " +
            QString::number(dataObject->input()->bounds()[5]));
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
    case s_btnClms + 3: return QVariant("x value range");
    case s_btnClms + 4: return QVariant("y value range");
    case s_btnClms + 5: return QVariant("z value range");
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
}
