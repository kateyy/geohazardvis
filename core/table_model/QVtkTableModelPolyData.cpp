#include "QVtkTableModelPolyData.h"

#include <cassert>
#include <QSet>

#include <vtkPolyData.h>
#include <vtkCellData.h>
#include <vtkCell.h>
#include <vtkPolygon.h>

#include <core/vtkhelper.h>
#include <core/data_objects/PolyDataObject.h>


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
        assert(m_polyData->cellCenters()->GetNumberOfPoints() > cellId);
        const double * centroid = m_polyData->cellCenters()->GetPoint(cellId);
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
    if (role != Qt::EditRole || index.column() < 2 || !m_vtkPolyData)
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

    vtkCell * cell = m_vtkPolyData->GetCell(cellId);
    vtkIdList * pointIds = cell->GetPointIds();


    // apply the value delta to all point of the triangle
    double point[3];
    for (int i = 0; i < pointIds->GetNumberOfIds(); ++i)
    {
        vtkIdType pointId = pointIds->GetId(i);
        m_vtkPolyData->GetPoint(pointId, point);
        point[component] += valueDelta;
        m_vtkPolyData->GetPoints()->SetPoint(pointId, point);
    }

    // also adjust the normals of adjacent triangles
    VTK_CREATE(vtkIdList, vertex);
    vertex->SetNumberOfIds(1);
    VTK_CREATE(vtkIdList, vertexNeighbors);

    QSet<vtkIdType> neighborCellIds;

    // find cell neighbors
    for (vtkIdType i = 0; i < pointIds->GetNumberOfIds(); ++i)
    {
        vertex->SetId(0, pointIds->GetId(i));
        m_vtkPolyData->GetCellNeighbors(cellId, vertex, vertexNeighbors);

        for (vtkIdType i = 0; i < vertexNeighbors->GetNumberOfIds(); ++i)
            neighborCellIds << vertexNeighbors->GetId(i);
    }

    vtkDataArray * normals = m_vtkPolyData->GetCellData()->GetNormals();
    assert(normals);

    for (vtkIdType neighborCellId : neighborCellIds)
    {
        vtkCell * neighbor = m_vtkPolyData->GetCell(neighborCellId);
    
        double normal[3];
        vtkPolygon::ComputeNormal(neighbor->GetPoints(), normal);
        normals->SetTuple(neighborCellId, normal);

        ++neighborCellId;
    }

    normals->Modified();
    m_vtkPolyData->Modified();

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
    if (m_polyData)
        m_vtkPolyData = vtkPolyData::SafeDownCast(dataObject()->dataSet());
    else
        m_vtkPolyData = nullptr;
}
