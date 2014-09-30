#include "QVtkTableModelPolyData.h"

#include <cassert>

#include <vtkPolyData.h>
#include <vtkCellData.h>
#include <vtkCell.h>

#include <core/data_objects/PolyDataObject.h>


QVtkTableModelPolyData::QVtkTableModelPolyData(QObject * parent)
: QVtkTableModel(parent)
{
}

int QVtkTableModelPolyData::rowCount(const QModelIndex &/*parent*/) const
{
    if (!m_polyData)
        return 0;

    return m_polyData->dataSet()->GetNumberOfCells();
}

int QVtkTableModelPolyData::columnCount(const QModelIndex &/*parent*/) const
{
    return 8;
}

QVariant QVtkTableModelPolyData::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !m_polyData)
        return QVariant();

    vtkIdType cellId = index.row();


    vtkSmartPointer<vtkDataSet> dataSet = m_polyData->dataSet();
    assert(dataSet->GetCell(cellId)->GetCellType() == VTKCellType::VTK_TRIANGLE);
    vtkCell * tri = dataSet->GetCell(cellId);
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
        vtkSmartPointer<vtkPolyData> centroids = m_polyData->cellCenters();
        assert(centroids->GetNumberOfPoints() > cellId);
        const double * centroid = centroids->GetPoint(cellId);
        int component = index.column() - 2;
        assert(component >= 0 && component < 3);
        return centroid[component];
    }
    case 5:
    case 6:
    case 7:
    {
        vtkDataArray * normals = m_polyData->processedDataSet()->GetCellData()->GetNormals();
        assert(normals);
        const double * normal = normals->GetTuple(cellId);
        int component = index.column() - 5;
        assert(component >= 0 && component < 3);
        return normal[component];
    }
    }

    return QVariant();
}

QVariant QVtkTableModelPolyData::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVtkTableModel::headerData(section, orientation, role);

    switch (section)
    {
    case 0: return "triangle ID";
    case 1: return "point IDs";
    case 2: return "x";
    case 3: return "y";
    case 4: return "z";
    case 5: return "normal x";
    case 6: return "normal y";
    case 7: return "normal z";
    }

    return QVariant();
}

bool QVtkTableModelPolyData::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole || index.column() < 2 || !m_polyData)
        return false;

    bool ok;
    double newValue = value.toDouble(&ok);
    if (!ok)
        return false;

    vtkIdType cellId = index.row();

    if (index.column() < 5)
    {
        int component = index.column() - 2;
        return m_polyData->setCellCenterComponent(cellId, component, newValue);
    }
    else if (index.column() < 8)
    {
        int component = index.column() - 5;
        return m_polyData->setCellNormalComponent(cellId, component, newValue);
    }

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
    m_polyData = dynamic_cast<PolyDataObject *>(dataObject());
}
