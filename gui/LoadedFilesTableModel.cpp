#include "LoadedFilesTableModel.h"

#include <cassert>

#include <core/Input.h>
#include <core/data_objects/DataObject.h>


LoadedFilesTableModel::LoadedFilesTableModel(QObject * parent)
    : QAbstractTableModel(parent)
{
}

int LoadedFilesTableModel::rowCount(const QModelIndex &/*parent = QModelIndex()*/) const
{
    return m_dataObjects.size();
}

int LoadedFilesTableModel::columnCount(const QModelIndex &/*parent = QModelIndex()*/) const
{
    return 1;
}

QVariant LoadedFilesTableModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
{
    if (role == Qt::DisplayRole && index.column() == 0)
    {
        assert(index.row() < m_dataObjects.size());
        return QVariant(QString::fromStdString(m_dataObjects[index.row()]->input()->name));
    }

    return QVariant();
}

QVariant LoadedFilesTableModel::headerData(int section, Qt::Orientation orientation,
    int /*role = Qt::DisplayRole*/) const
{
    assert(orientation == Qt::Orientation::Horizontal);

    if (section == 0)
        return QVariant("data set name");

    return QVariant();
}

void LoadedFilesTableModel::addDataObject(DataObject * dataObject)
{
    beginInsertRows(QModelIndex(), m_dataObjects.size(), m_dataObjects.size() + 1);
    m_dataObjects << dataObject;
    endInsertRows();
}