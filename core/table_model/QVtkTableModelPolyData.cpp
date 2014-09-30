#include "QVtkTableModelPolyData.h"

#include <cassert>

#include <vtkPolyData.h>
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
    return 5;
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
    }

    return QVariant();
}

bool QVtkTableModelPolyData::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole || index.column() < 2 || !m_polyData)
        return false;

    // get the delta between current and changed centroid coordinate value
    bool ok;
    double formerValue = data(index).toDouble(&ok);
    assert(ok);
    double newValue = value.toDouble(&ok);
    if (!ok)
        return false;

    double valueDelta = value.toDouble(&ok) - formerValue;


    vtkIdType cellId = index.row();
    int component = index.column() - 2;
    assert(component >= 0 && component < 3);

    vtkSmartPointer<vtkPolyData> dataSet = vtkPolyData::SafeDownCast(m_polyData->dataSet());
    assert(dataSet);

    vtkCell * cell = dataSet->GetCell(cellId);
    vtkIdList * pointIds = cell->GetPointIds();


    // apply the value delta to all point of the triangle
    double point[3];
    for (int i = 0; i < pointIds->GetNumberOfIds(); ++i)
    {
        vtkIdType pointId = pointIds->GetId(i);
        dataSet->GetPoint(pointId, point);
        point[component] += valueDelta;
        dataSet->GetPoints()->SetPoint(pointId, point);
    }

    dataSet->Modified();

    return true;
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
