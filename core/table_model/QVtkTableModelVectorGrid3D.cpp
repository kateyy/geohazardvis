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
    return 7;
}

QVariant QVtkTableModelVectorGrid3D::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !m_gridData)
        return QVariant();

    vtkIdType pointId = index.row();

    const double * point = m_gridData->GetPoint(pointId);

    switch (index.column())
    {
    case 0:
        return pointId;
    case 1:
    case 2:
    case 3:
        return point[index.column() - 1];
    case 4:
    case 5:
    case 6:
    {
        vtkDataArray * vectors = m_gridData->GetPointData()->GetScalars();
        assert(vectors && vectors->GetNumberOfComponents() == 3);
        return vectors->GetTuple(pointId)[index.column() - 4];
    }
    }

    return QVariant();
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
    case 4: return "vector x";
    case 5: return "vector y";
    case 6: return "vector z";
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

    vtkIdType vectorId = index.row();

    vtkDataArray * data = m_gridData->GetPointData()->GetScalars();
    assert(data);

    int numComponents = data->GetNumberOfComponents();
    int componentId = index.column() - 4;
    assert(componentId >= 0 && componentId < numComponents);


    std::vector<double> tuple(numComponents);

    data->GetTuple(vectorId, tuple.data());
    tuple[componentId] = newValue;
    data->SetTuple(vectorId, tuple.data());
    data->Modified();
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
}
