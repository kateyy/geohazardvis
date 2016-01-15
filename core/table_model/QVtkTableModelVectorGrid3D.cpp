#include "QVtkTableModelVectorGrid3D.h"

#include <cassert>

#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkCell.h>

#include <core/types.h>
#include <core/data_objects/DataObject.h>


QVtkTableModelVectorGrid3D::QVtkTableModelVectorGrid3D(QObject * parent)
    : QVtkTableModel(parent)
    , m_gridData(nullptr)
    , m_scalars(nullptr)
{
}

int QVtkTableModelVectorGrid3D::rowCount(const QModelIndex &/*parent*/) const
{
    if (!m_gridData)
        return 0;

    return static_cast<int>(m_gridData->GetNumberOfPoints());
}

int QVtkTableModelVectorGrid3D::columnCount(const QModelIndex &/*parent*/) const
{
    if (!m_gridData)
    {
        return 0;
    }


    int columnCount = 1 + 3
        + (m_scalars ? m_scalars->GetNumberOfComponents() : 0);

    return columnCount;
    
}

QVariant QVtkTableModelVectorGrid3D::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !m_gridData)
        return QVariant();

    const vtkIdType pointId = index.row();

    if (index.column() == 0)
    {
        return pointId;
    }

    if (index.column() < 3)
    {
        const double * point = m_gridData->GetPoint(pointId);
        return point[index.column() - 1];
    }

    const int component = index.column() - 4;
    assert(m_scalars && (m_scalars->GetNumberOfComponents() > component));
    return m_scalars->GetTuple(pointId)[component];
}

QVariant QVtkTableModelVectorGrid3D::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVtkTableModel::headerData(section, orientation, role);

    switch (section)
    {
    case 0: return "ID";
    case 1: return "point x";
    case 2: return "point y";
    case 3: return "point z";
    default:
        assert(m_scalars);
        const int component = section - 4;
        if (m_scalars->GetNumberOfComponents() == 1)
        {
            return "data";
        }
        if (m_scalars->GetNumberOfComponents() == 3)
        {
            return "vector " + ('x' + component);
        }
        return "data " + QString::number(component + 1);
    }

    return QVariant();
}

bool QVtkTableModelVectorGrid3D::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole || index.column() < 4 || !m_gridData)
        return false;

    bool validValue;
    double newValue = value.toDouble(&validValue);
    if (!validValue)
        return false;

    const vtkIdType tupleId = index.row();

    int numComponents = m_scalars->GetNumberOfComponents();
    int componentId = index.column() - 4;
    assert(componentId >= 0 && componentId < numComponents);


    std::vector<double> tuple(numComponents);

    m_scalars->GetTuple(tupleId, tuple.data());
    tuple[componentId] = newValue;
    m_scalars->SetTuple(tupleId, tuple.data());
    m_scalars->Modified();
    m_gridData->Modified();

    return false;
}

Qt::ItemFlags QVtkTableModelVectorGrid3D::flags(const QModelIndex &index) const
{
    if (index.column() > 3)
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);

    return QAbstractItemModel::flags(index);
}

IndexType QVtkTableModelVectorGrid3D::indexType() const
{
    return IndexType::points;
}

void QVtkTableModelVectorGrid3D::resetDisplayData()
{
    m_gridData = dataObject() ? dataObject()->dataSet() : nullptr;

    if (m_gridData)
    {
        m_scalars = m_gridData->GetPointData()->GetScalars();
    }
}
