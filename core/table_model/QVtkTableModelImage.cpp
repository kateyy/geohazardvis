#include "QVtkTableModelImage.h"

#include <cassert>

#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

#include <core/types.h>
#include <core/data_objects/DataObject.h>


namespace
{
inline int positive_modulo(int i, int n)
{
    return (i % n + n) % n;
}

const int c_firstValueColumn = 2;
}


QVtkTableModelImage::QVtkTableModelImage(QObject * parent)
    : QVtkTableModel(parent)
{
}

int QVtkTableModelImage::rowCount(const QModelIndex &/*parent*/) const
{
    if (!m_vtkImageData)
        return 0;

    return static_cast<int>(m_vtkImageData->GetNumberOfPoints());
}

int QVtkTableModelImage::columnCount(const QModelIndex &/*parent*/) const
{
    return 2 + m_vtkImageData->GetNumberOfScalarComponents();
}

QVariant QVtkTableModelImage::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !m_vtkImageData)
        return QVariant();

    int imageRow, imageColumn;
    tableToImageCoord(index.row(), imageRow, imageColumn);

    // image row/column columns
    if (index.column() == 0)
        return imageRow;
    if (index.column() == 1)
        return imageColumn;

    // scalar data columns
    int component = index.column() - 2;
    assert(component < m_vtkImageData->GetNumberOfScalarComponents());
    return m_vtkImageData->GetScalarComponentAsDouble(imageRow, imageColumn, 0, component);
}

QVariant QVtkTableModelImage::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVtkTableModel::headerData(section, orientation, role);

    switch (section)
    {
    case 0: return "row (x)";
    case 1: return "column (y)";
    default: // value columns
        assert(section >= c_firstValueColumn);
        if (m_vtkImageData->GetNumberOfScalarComponents() > 1)
            return "value (" + QString::number(section - c_firstValueColumn) + ")";
        return "value";
    }
}

bool QVtkTableModelImage::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole || index.column() < c_firstValueColumn || !m_vtkImageData)
        return false;

    bool ok;
    double f_value = value.toDouble(&ok);
    if (!ok)
        return false;

    int imageRow, imageColumn;
    tableToImageCoord(index.row(), imageRow, imageColumn);

    int component = index.column() - c_firstValueColumn;
    assert(component < m_vtkImageData->GetNumberOfScalarComponents());

    m_vtkImageData->SetScalarComponentFromDouble(imageRow, imageColumn, 0, component, f_value);
    // SetScalarComponentFromDouble does not set the scalar array as modified, so that the pipeline won't be updated
    m_vtkImageData->GetPointData()->GetScalars()->Modified();

    return true;
}

Qt::ItemFlags QVtkTableModelImage::flags(const QModelIndex &index) const
{
    if (index.column() >= c_firstValueColumn)
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);

    return QAbstractItemModel::flags(index);
}

IndexType QVtkTableModelImage::indexType() const
{
    return IndexType::points;
}

void QVtkTableModelImage::resetDisplayData()
{
    m_vtkImageData = vtkImageData::SafeDownCast(dataObject()->dataSet());
}

void QVtkTableModelImage::tableToImageCoord(int tableRow, int & imageRow, int & imageColumn) const
{
    imageRow = positive_modulo(tableRow, m_vtkImageData->GetDimensions()[0]);
    imageColumn = tableRow / m_vtkImageData->GetDimensions()[0];
}
