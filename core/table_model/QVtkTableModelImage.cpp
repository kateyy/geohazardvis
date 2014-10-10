#include "QVtkTableModelImage.h"

#include <cassert>

#include <vtkImageData.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkCell.h>

#include <core/data_objects/DataObject.h>


QVtkTableModelImage::QVtkTableModelImage(QObject * parent)
    : QVtkTableModel(parent)
{
}

int QVtkTableModelImage::rowCount(const QModelIndex &/*parent*/) const
{
    if (!m_vtkImageData)
        return 0;

    return m_vtkImageData->GetCellData()->GetNumberOfTuples();
}

int QVtkTableModelImage::columnCount(const QModelIndex &/*parent*/) const
{
    return 4;
}

QVariant QVtkTableModelImage::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !m_vtkImageData)
        return QVariant();

    vtkIdType cellId = index.row();
    vtkIdType firstPointId = m_vtkImageData->GetCell(cellId)->GetPointId(0);
    const double * position = m_vtkImageData->GetPoint(firstPointId);

    switch (index.column())
    {
    case 0:
        return cellId;
    case 1:
        return static_cast<long long int>(position[0]);
    case 2:
        return static_cast<long long int>(position[1]);
    case 3:
        vtkDataArray * data = m_vtkImageData->GetCellData()->GetScalars();
        assert(data);
        return data->GetTuple(cellId)[0];
    }

    return QVariant();
}

QVariant QVtkTableModelImage::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVtkTableModel::headerData(section, orientation, role);

    switch (section)
    {
    case 0: return "ID";
    case 1: return "x";
    case 2: return "y";
    case 3: return "value";
    }

    return QVariant();
}

bool QVtkTableModelImage::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole || index.column() != 3 || !m_vtkImageData)
        return false;

    vtkDataArray * data = m_vtkImageData->GetCellData()->GetScalars();
    assert(data);

    bool ok;
    double f_value = value.toDouble(&ok);
    if (!ok)
        return false;

    data->SetVariantValue(index.row(), f_value);
    data->Modified();
    
    return true;
}

Qt::ItemFlags QVtkTableModelImage::flags(const QModelIndex &index) const
{
    if (index.column() == 3)
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);

    return QAbstractItemModel::flags(index);
}

void QVtkTableModelImage::resetDisplayData()
{
    m_vtkImageData = vtkImageData::SafeDownCast(dataObject()->dataSet());
}
