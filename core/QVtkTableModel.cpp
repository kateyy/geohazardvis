#include "QVtkTableModel.h"

#include <QDebug>

#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkTriangle.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>


QVtkTableModel::QVtkTableModel(QObject * parent)
: QAbstractTableModel(parent)
, m_vtkPolyData(nullptr)
, m_vtkImageData(nullptr)
, m_displayData(DisplayData::Triangles)
{
}

int QVtkTableModel::rowCount(const QModelIndex &parent) const
{
    switch (m_displayData) {
    case DisplayData::Triangles:
        if (m_vtkPolyData == nullptr)
            return 0;
        return m_vtkPolyData->GetPolys()->GetNumberOfCells();
    case DisplayData::Grid:
        if (m_vtkImageData == nullptr)
            return 0;
        return m_vtkImageData->GetPointData()->GetNumberOfTuples();
    }
    return 0;
}

int QVtkTableModel::columnCount(const QModelIndex &parent) const
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
            return QVariant(
                QString::number(tri->GetPointId(0)) + ":" +
                QString::number(tri->GetPointId(1)) + ":" +
                QString::number(tri->GetPointId(2)));
        }

        // list on of the triangle points for now
        return QVariant(tri->GetPoints()->GetPoint(0)[index.column() - 1]);
    }
    if (m_displayData == DisplayData::Grid) {
        if (m_vtkImageData == nullptr)
            return QVariant();
        vtkFloatArray * values = vtkFloatArray::SafeDownCast(m_vtkImageData->GetPointData()->GetScalars());
        return QVariant(*values->GetTuple(index.row()));
    }
    return QVariant();
}

QVariant QVtkTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Orientation::Vertical)
        return QVariant(section);
    if (section == 0)
        return QVariant("indices");
    if (m_displayData == DisplayData::Triangles) {
        return QVariant(QString(QChar('x' + section - 1)));
    }
    if (m_displayData == DisplayData::Grid) {
        return QVariant("value");
    }
    return QVariant();
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
    
    qDebug() << "Error: receiving unsupported data format in table model";
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
