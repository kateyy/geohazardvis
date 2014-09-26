#include "QVtkTableModelPolyData.h"

#include <cassert>

#include <vtkPolyData.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkCell.h>

#include <core/data_objects/DataObject.h>


QVtkTableModelPolyData::QVtkTableModelPolyData(QObject * parent)
: QVtkTableModel(parent)
, m_vtkPolyData(nullptr)
{
}

int QVtkTableModelPolyData::rowCount(const QModelIndex &/*parent*/) const
{
    if (!m_vtkPolyData)
        return 0;

    return m_vtkPolyData->GetNumberOfCells();
}

int QVtkTableModelPolyData::columnCount(const QModelIndex &/*parent*/) const
{
    return 5;
}

QVariant QVtkTableModelPolyData::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || ! m_vtkPolyData)
        return QVariant();

    vtkIdType cellId = index.row();

    assert(m_vtkPolyData->GetCell(cellId)->GetCellType() == VTKCellType::VTK_TRIANGLE);
    vtkCell * tri = m_vtkPolyData->GetCell(cellId);
    assert(tri);

    switch (index.column())
    {
    case 0:
        return cellId;
    case 1:
        return
            QString::number(tri->GetPointId(0)) + ":" +
            QString::number(tri->GetPointId(1)) + ":" +
            QString::number(tri->GetPointId(2));
    case 2:
    case 3:
    case 4:
    {
        vtkDataArray * centroid = m_vtkPolyData->GetCellData()->GetArray("centroid");
        assert(centroid);
        return centroid->GetTuple(cellId)[index.column() - 2];
    }
    }

    return QVariant();
}

QVariant QVtkTableModelPolyData::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section)
    {
    case 0: return "triangle ID";
    case 1: return "point IDs";
    case 2: return "x";
    case 3: return "y";
    case 4: return "z";
    }

    return QVariant();
}

bool QVtkTableModelPolyData::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole || index.column() != 5 || !m_vtkPolyData)
        return false;

    /** TODO */

    return false;
}

Qt::ItemFlags QVtkTableModelPolyData::flags(const QModelIndex &index) const
{
    if (index.column() > 1)
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);

    return QAbstractItemModel::flags(index);
}

void QVtkTableModelPolyData::resetDisplayData()
{
    m_vtkPolyData = vtkPolyData::SafeDownCast(dataObject()->dataSet());
}
