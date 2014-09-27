#include "QVtkTableModelRawVector.h"

#include <cassert>

#include <vtkFloatArray.h>

#include <core/data_objects/AttributeVectorData.h>


QVtkTableModelRawVector::QVtkTableModelRawVector(QObject * parent)
    : QVtkTableModel(parent)
{
}

int QVtkTableModelRawVector::rowCount(const QModelIndex &/*parent*/) const
{
    if (!m_data)
        return 0;

    return 1 + m_data->GetNumberOfTuples();
}

int QVtkTableModelRawVector::columnCount(const QModelIndex &/*parent*/) const
{
    if (!m_data)
        return 0;

    return 1 + m_data->GetNumberOfComponents();
}

QVariant QVtkTableModelRawVector::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !m_data)
        return QVariant();

    vtkIdType tupleId = index.row();

    if (index.column() == 0)
        return tupleId;
    
    vtkIdType component = index.column() - 1;
    assert(component < m_data->GetNumberOfComponents());
    
    return m_data->GetTuple(tupleId)[component];
}

QVariant QVtkTableModelRawVector::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    if (section == 0)
        return "ID";

    vtkIdType components = m_data->GetNumberOfComponents();

    switch (components)
    {
    case 1:
        return "value";
    case 2:
    case 3:
        return QChar('x' + section - 1);
    default:
        return section - 1;
    }
}

bool QVtkTableModelRawVector::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole || index.column() == 0 || !m_data)
        return false;

    bool ok;
    double f_value = value.toDouble(&ok);
    if (!ok)
        return false;

    vtkIdType tupleId = index.row(), component = index.column() - 1;
    int components = m_data->GetNumberOfComponents();

    assert(component < components && tupleId < m_data->GetNumberOfTuples());

    double * tupleData = new double[components];

    m_data->GetTuple(tupleId, tupleData);
    tupleData[component] = f_value;
    m_data->SetTuple(tupleId, tupleData);

    delete tupleData;

    m_data->Modified();

    return true;
}

Qt::ItemFlags QVtkTableModelRawVector::flags(const QModelIndex &index) const
{
    if (index.column() > 0)
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);

    return QAbstractItemModel::flags(index);
}

void QVtkTableModelRawVector::resetDisplayData()
{
    if (AttributeVectorData * dataVector = dynamic_cast<AttributeVectorData *>(dataObject()))
        m_data = dataVector->dataArray();
    else
        m_data = nullptr;
}
