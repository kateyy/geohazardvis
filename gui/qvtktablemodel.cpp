#include "qvtktablemodel.h"

#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkTriangle.h>

QVtkTableModel::QVtkTableModel(QObject * parent)
: QAbstractTableModel(parent)
, m_vtkData(nullptr)
, m_displayData(DisplayData::Polygons)
{
}

int QVtkTableModel::rowCount(const QModelIndex &parent) const
{
    if (m_vtkData == nullptr)
        return 0;
    switch (m_displayData) {
    case DisplayData::Points:
        return m_vtkData->GetPoints()->GetNumberOfPoints();
    case DisplayData::Polygons:
        return m_vtkData->GetPolys()->GetNumberOfCells();
    }
    return 0;
}

int QVtkTableModel::columnCount(const QModelIndex &parent) const
{
    if (m_vtkData == nullptr)
        return 0;
    switch (m_displayData) {
    case DisplayData::Points:
        return 3; // xyz
    case DisplayData::Polygons:
        return 4; // xyz(triangle center, indices)
    }
    return 0;
}

QVariant QVtkTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();
    if (m_displayData == DisplayData::Points) {
        assert(index.column() < 3);
        return m_vtkData->GetPoints()->GetPoint(index.row())[index.column()];
    }
    if (m_displayData == DisplayData::Polygons) {
        assert(index.column() < 4);
        assert(m_vtkData->GetCell(index.row())->GetCellType() == VTKCellType::VTK_TRIANGLE);
        vtkTriangle * tri = static_cast<vtkTriangle*>(m_vtkData->GetCell(index.row()));
        assert(tri);

        if (index.column() == 0) {  // list indices
            return QVariant(
                QString::number(tri->GetPointId(0)) + ":" +
                QString::number(tri->GetPointId(1)) + ":" +
                QString::number(tri->GetPointId(2)));
        }
        vtkPoints * pts = tri->GetPoints();
        double center[3];
        vtkTriangle::TriangleCenter(pts->GetPoint(0), pts->GetPoint(1), pts->GetPoint(2), center);
        return QVariant(center[index.column() - 1]);
    }
    return QVariant();
}

QVariant QVtkTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Orientation::Vertical)
        return QVariant(section);
    if (m_displayData == DisplayData::Points)
        return QVariant(QChar('x' + section));
    if (m_displayData == DisplayData::Polygons) {
        if (section == 0)
            return QVariant("indices");
        return QVariant("center." + QString(QChar('x' + section - 1)));
    }
    return QVariant();
}

void QVtkTableModel::showPolyData(vtkSmartPointer<vtkPolyData> data)
{
    beginResetModel();
    m_vtkData = data;
    endResetModel();
}
