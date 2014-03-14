#include "qvtktablemodel.h"

#include <vtkPolyData.h>

QVtkTableModel::QVtkTableModel(QObject * parent)
: QAbstractTableModel(parent)
{
}

int QVtkTableModel::rowCount(const QModelIndex &parent) const
{
    if (m_vtkData == nullptr)
        return 0;
    return m_vtkData->GetPoints()->GetNumberOfPoints();
}

int QVtkTableModel::columnCount(const QModelIndex &parent) const
{
    if (m_vtkData == nullptr)
        return 0;
    return 3;
}

QVariant QVtkTableModel::data(const QModelIndex &index, int role) const
{
    assert(index.column() < 3);
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();
    double * vertex = m_vtkData->GetPoints()->GetPoint(index.row());
    return QVariant(vertex[index.column()]);
}

QVariant QVtkTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Orientation::Vertical)
        return QVariant(section);
    return QVariant(QChar('x' + section));
}

void QVtkTableModel::showPolyData(vtkSmartPointer<vtkPolyData> data)
{
    beginResetModel();
    m_vtkData = data;
    endResetModel();
}
