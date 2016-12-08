#include "QVtkTableModel.h"

#include <cassert>

#include <core/data_objects/DataObject.h>
#include <core/utility/qthelper.h>


QVtkTableModel::QVtkTableModel(QObject * parent)
    : QAbstractTableModel(parent)
    , m_dataObject{ nullptr }
    , m_hightlightId{ -1 }
{
}

QVtkTableModel::~QVtkTableModel() = default;

QVariant QVtkTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Vertical)
    {
        return QVariant();
    }

    if (section == m_hightlightId)
    {
        return QChar(0x25CF);
    }


    return QVariant();
}

void QVtkTableModel::setDataObject(DataObject * dataObject)
{
    assert(dataObject);

    if (m_dataObject == dataObject)
    {
        return;
    }

    m_dataObject = dataObject;

    rebuild();
}

void QVtkTableModel::rebuild()
{
    disconnectAll(m_dataObjectConnections);

    beginResetModel();

    m_hightlightId = -1;

    resetDisplayData();

    endResetModel();

    m_dataObjectConnections.emplace_back(connect(m_dataObject, &DataObject::structureChanged,
        this, &QVtkTableModel::rebuild));
}

DataObject * QVtkTableModel::dataObject()
{
    return m_dataObject;
}

vtkIdType QVtkTableModel::hightlightItemId() const
{
    return m_hightlightId;
}

vtkIdType QVtkTableModel::itemIdAt(const QModelIndex & index) const
{
    return index.row();
}

void QVtkTableModel::setHighlightItemId(vtkIdType id)
{
    if (m_hightlightId == id)
    {
        return;
    }

    m_hightlightId = id;

    headerDataChanged(Qt::Vertical, 0, rowCount());
}
