#include "QVtkTableModelPointCloudData.h"

#include <cassert>

#include <core/types.h>
#include <core/data_objects/GenericPolyDataObject.h>


QVtkTableModelPointCloudData::QVtkTableModelPointCloudData(QObject * parent)
    : QVtkTableModel(parent)
    , m_data{ nullptr }
{
}

QVtkTableModelPointCloudData::~QVtkTableModelPointCloudData() = default;

int QVtkTableModelPointCloudData::rowCount(const QModelIndex &/*parent*/) const
{
    if (!m_data)
    {
        return 0;
    }

    return static_cast<int>(m_data->numberOfPoints());
}

int QVtkTableModelPointCloudData::columnCount(const QModelIndex &/*parent*/) const
{
    return 4;
}

QVariant QVtkTableModelPointCloudData::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !m_data)
    {
        return{};
    }

    const auto pointId = static_cast<vtkIdType>(index.row());

    switch (index.column())
    {
    case 0:
        return pointId;
    case 1:
    case 2:
    case 3:
    {
        const int component = index.column() - 1;
        assert(component >= 0 && component < 3);
        bool okay;
        const auto value = m_data->pointCoordinateComponent(pointId, component, &okay);
        assert(okay);
        if (okay)
        {
            return value;
        }
        return{};
    }
    }

    return{};
}

QVariant QVtkTableModelPointCloudData::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
    {
        return QVtkTableModel::headerData(section, orientation, role);
    }

    switch (section)
    {
    case 0: return "Point ID";
    case 1: return "x";
    case 2: return "y";
    case 3: return "z";
    }

    return QVariant();
}

bool QVtkTableModelPointCloudData::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole || index.column() < 1 || !m_data)
    {
        return false;
    }

    bool ok;
    const double newValue = value.toDouble(&ok);
    if (!ok)
    {
        return false;
    }

    const auto pointId = static_cast<vtkIdType>(index.row());
    const int component = index.column() - 1;
    assert(component >= 0 && component < 3);

    return m_data->setPointCoordinateComponent(pointId, component, newValue);
}

Qt::ItemFlags QVtkTableModelPointCloudData::flags(const QModelIndex &index) const
{
    if (index.column() > 1)
    {
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
    }

    return QAbstractItemModel::flags(index);
}

IndexType QVtkTableModelPointCloudData::indexType() const
{
    return IndexType::points;
}

void QVtkTableModelPointCloudData::resetDisplayData()
{
    m_data = dynamic_cast<GenericPolyDataObject *>(dataObject());
}
