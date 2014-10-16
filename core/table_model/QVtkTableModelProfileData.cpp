#include "QVtkTableModelProfileData.h"

#include <cassert>

#include <vtkDataSet.h>

#include <core/data_objects/ImageProfileData.h>


QVtkTableModelProfileData::QVtkTableModelProfileData(QObject * parent)
    : QVtkTableModel(parent)
{
}

int QVtkTableModelProfileData::rowCount(const QModelIndex &/*parent*/) const
{
    if (!m_data)
        return 0;

    return m_data->numberOfScalars();
}

int QVtkTableModelProfileData::columnCount(const QModelIndex &/*parent*/) const
{
    return 2;
}

QVariant QVtkTableModelProfileData::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !m_data)
        return QVariant();

    vtkIdType valueId = index.row();

    int positionOrValue = index.column();

    if (positionOrValue > 1)
        return QVariant();

    double point[3];
    m_data->processedDataSet()->GetPoint(valueId, point);
    
    return point[positionOrValue];
}

QVariant QVtkTableModelProfileData::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVtkTableModel::headerData(section, orientation, role);

    switch (section)
    {
    case 0: return "position";
    case 1:
        if (m_data)
            return m_data->scalarsName();
        return "value";
    }

    return QVariant();
}

void QVtkTableModelProfileData::resetDisplayData()
{
    m_data = dynamic_cast<ImageProfileData *>(dataObject());
}
