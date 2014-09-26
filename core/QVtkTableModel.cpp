#include "QVtkTableModel.h"

#include <cassert>

#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkTriangle.h>
#include <vtkImageData.h>
#include <vtkCellData.h>
#include <vtkFloatArray.h>


QVtkTableModel::QVtkTableModel(QObject * parent)
: QAbstractTableModel(parent)
, m_vtkPolyData(nullptr)
, m_vtkImageData(nullptr)
, m_displayData(DisplayData::Triangles)
{
}

int QVtkTableModel::rowCount(const QModelIndex &/*parent*/) const
{
    switch (m_displayData) {
    case DisplayData::Triangles:
        if (m_vtkPolyData == nullptr)
            return 0;
        return m_vtkPolyData->GetPolys()->GetNumberOfCells();
    case DisplayData::Grid:
        if (m_vtkImageData == nullptr)
            return 0;
        return m_vtkImageData->GetCellData()->GetNumberOfTuples();
    }
    return 0;
}

int QVtkTableModel::columnCount(const QModelIndex &/*parent*/) const
{
    switch (m_displayData) {
    case DisplayData::Triangles:
        return 4; // xyz(triangle center, indices)
    case DisplayData::Grid:
        return 1;
    }
    return 0;
}

QVariant QVtkTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();
    if (m_displayData == DisplayData::Triangles) {
        if (m_vtkPolyData == nullptr)
            return QVariant();
        assert(index.column() < 4);
        assert(m_vtkPolyData->GetCell(index.row())->GetCellType() == VTKCellType::VTK_TRIANGLE);
        vtkTriangle * tri = static_cast<vtkTriangle*>(m_vtkPolyData->GetCell(index.row()));
        assert(tri);

        if (index.column() == 0) {  // list indices
            return
                QString::number(tri->GetPointId(0)) + ":" +
                QString::number(tri->GetPointId(1)) + ":" +
                QString::number(tri->GetPointId(2));
        }

        // list on of the triangle points for now
        return tri->GetPoints()->GetPoint(0)[index.column() - 1];
    }
    if (m_displayData == DisplayData::Grid) {
        if (m_vtkImageData == nullptr)
            return QVariant();
        vtkFloatArray * values = vtkFloatArray::SafeDownCast(m_vtkImageData->GetCellData()->GetScalars());
        return *values->GetTuple(index.row());
    }
    return QVariant();
}

QVariant QVtkTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Orientation::Vertical)
        return section;
    if (section == 0)
        return "indices";
    if (m_displayData == DisplayData::Triangles) {
        return QChar('x' + section - 1);
    }
    if (m_displayData == DisplayData::Grid) {
        return "value";
    }
    return QVariant();
}

bool QVtkTableModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole)
        return false;

    if (m_displayData != DisplayData::Grid)
        return false;

    vtkDataArray * scalars = m_vtkImageData->GetCellData()->GetScalars();
    assert(scalars);

    bool ok;
    float f_value = value.toFloat(&ok);
    if (!ok)
        return false;

    scalars->SetVariantValue(index.row(), f_value);
    scalars->Modified();

    emit dataChanged();
    
    return true;
}

Qt::ItemFlags QVtkTableModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

void QVtkTableModel::showData(vtkDataSet * data)
{
    switch (data->GetDataObjectType())
    {
    case VTK_POLY_DATA:
        return showPolyData(static_cast<vtkPolyData*>(data));
    case VTK_IMAGE_DATA:
        return showGridData(static_cast<vtkImageData*>(data));
    }
    
    assert(false);
}

void QVtkTableModel::showPolyData(vtkPolyData * polyData)
{
    beginResetModel();
    m_vtkPolyData = polyData;
    m_vtkImageData = nullptr;
    m_displayData = DisplayData::Triangles;
    endResetModel();
}

void QVtkTableModel::showGridData(vtkImageData * gridData)
{
    beginResetModel();
    m_vtkPolyData = nullptr;
    m_vtkImageData = gridData;
    m_displayData = DisplayData::Grid;
    endResetModel();
}
