#include "DataBrowserTableModel.h"

#include <cassert>

#include <vtkDataSet.h>

#include <core/Input.h>
#include <core/data_objects/DataObject.h>


DataBrowserTableModel::DataBrowserTableModel(QObject * parent)
    : QAbstractTableModel(parent)
{
}

int DataBrowserTableModel::rowCount(const QModelIndex &/*parent = QModelIndex()*/) const
{
    return m_dataObjects.size();
}

int DataBrowserTableModel::columnCount(const QModelIndex &/*parent = QModelIndex()*/) const
{
    return 6;
}

QVariant DataBrowserTableModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    assert(index.row() < m_dataObjects.size());

    DataObject * dataObject = m_dataObjects[index.row()];

    switch (index.column())
    {
    case 0:
        return QVariant(QString::fromStdString(dataObject->input()->name));
    case 1:
        return QVariant(dataObject->dataTypeName());
    case 2:
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
    case 3:
        return QVariant(
            QString::number(dataObject->input()->bounds()[0]) + "; " +
            QString::number(dataObject->input()->bounds()[1]));
    case 4:
        return QVariant(
            QString::number(dataObject->input()->bounds()[2]) + "; " +
            QString::number(dataObject->input()->bounds()[3]));
    case 5:
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
    case 0: return QVariant("name");
    case 1: return QVariant("data set type");
    case 2: return QVariant("dimensions");
    case 3: return QVariant("x value range");
    case 4: return QVariant("y value range");
    case 5: return QVariant("z value range");
    }        

    return QVariant();
}

DataObject * DataBrowserTableModel::dataObjectAt(int row)
{
    if (row < 0 || row >= m_dataObjects.size())
    {
        assert(false);
        return nullptr;
    }

    return m_dataObjects.at(row);
}

void DataBrowserTableModel::addDataObject(DataObject * dataObject)
{
    beginInsertRows(QModelIndex(), m_dataObjects.size(), m_dataObjects.size());
    m_dataObjects << dataObject;
    endInsertRows();
}

void DataBrowserTableModel::removeDataObject(DataObject * dataObject)
{
    int index = m_dataObjects.indexOf(dataObject);
    beginRemoveRows(QModelIndex(), index, index);
    m_dataObjects.removeAt(index);
    endRemoveRows();
}
