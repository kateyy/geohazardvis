#include "QVtkTableModelProfileData.h"

#include <array>
#include <cassert>

#include <vtkDataSet.h>

#include <core/types.h>
#include <core/data_objects/DataProfile2DDataObject.h>


QVtkTableModelProfileData::QVtkTableModelProfileData(QObject * parent)
    : QVtkTableModel(parent)
    , m_data{ nullptr }
{
}

QVtkTableModelProfileData::~QVtkTableModelProfileData() = default;

int QVtkTableModelProfileData::rowCount(const QModelIndex &/*parent*/) const
{
    if (!m_data)
    {
        return 0;
    }

    return m_data->numberOfScalars();
}

int QVtkTableModelProfileData::columnCount(const QModelIndex &/*parent*/) const
{
    return 2;
}

QVariant QVtkTableModelProfileData::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !m_data)
    {
        return QVariant();
    }

    const vtkIdType valueId = index.row();
    const int positionOrValue = index.column();

    if (positionOrValue > 1)
    {
        return QVariant();
    }

    std::array<double, 3> point;
    m_data->processedDataSet()->GetPoint(valueId, point.data());
    
    return point[positionOrValue];
}

QVariant QVtkTableModelProfileData::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
    {
        return QVtkTableModel::headerData(section, orientation, role);
    }

    switch (section)
    {
    case 0: return "Position";
    case 1:
        if (m_data)
        {
            return m_data->scalarsName();
        }
        return "Value";
    }

    return QVariant();
}

IndexType QVtkTableModelProfileData::indexType() const
{
    return IndexType::points;
}

void QVtkTableModelProfileData::resetDisplayData()
{
    m_data = dynamic_cast<DataProfile2DDataObject *>(dataObject());
}
