#include "QVtkTableModelPolyData.h"

#include <array>
#include <cassert>

#include <vtkPolyData.h>
#include <vtkCellData.h>
#include <vtkCell.h>
#include <vtkCellTypes.h>

#include <core/types.h>
#include <core/data_objects/PolyDataObject.h>


QVtkTableModelPolyData::QVtkTableModelPolyData(QObject * parent)
    : QVtkTableModel(parent)
    , m_polyData{ nullptr }
{
}

QVtkTableModelPolyData::~QVtkTableModelPolyData() = default;

int QVtkTableModelPolyData::rowCount(const QModelIndex &/*parent*/) const
{
    if (!m_polyData)
    {
        return 0;
    }

    return static_cast<int>(m_polyData->dataSet()->GetNumberOfCells());
}

int QVtkTableModelPolyData::columnCount(const QModelIndex &/*parent*/) const
{
    return 8;
}

QVariant QVtkTableModelPolyData::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !m_polyData)
    {
        return QVariant();
    }

    vtkIdType cellId = index.row();

    switch (index.column())
    {
    case 0:
        return cellId;
    case 1:
    {
        vtkCell * cell = m_polyData->dataSet()->GetCell(cellId);
        assert(cell);
        QString idListString;
        for (auto i = 0; i < cell->GetNumberOfPoints(); ++i)
        {
            idListString += QString::number(cell->GetPointId(i)) + ":";
        }

        return idListString.left(idListString.length() - 1);
    }
    case 2:
    case 3:
    case 4:
    {
        vtkSmartPointer<vtkPolyData> centroids = m_polyData->cellCenters();
        assert(centroids->GetNumberOfPoints() > cellId);
        std::array<double, 3> centroid;
        centroids->GetPoint(cellId, centroid.data());
        const int component = index.column() - 2;
        assert(component >= 0 && component < 3);
        return centroid[component];
    }
    case 5:
    case 6:
    case 7:
    {
        vtkDataArray * normals = m_polyData->processedDataSet()->GetCellData()->GetNormals();
        assert(normals);
        std::array<double, 3> normal;
        normals->GetTuple(cellId, normal.data());
        const int component = index.column() - 5;
        assert(component >= 0 && component < 3);
        return normal[component];
    }
    }

    return QVariant();
}

QVariant QVtkTableModelPolyData::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
    {
        return QVtkTableModel::headerData(section, orientation, role);
    }

    switch (section)
    {
    case 0: return m_cellTypeName + " ID";
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
    {
        return false;
    }

    bool ok;
    const double newValue = value.toDouble(&ok);
    if (!ok)
    {
        return false;
    }

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
    {
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
    }

    return QAbstractItemModel::flags(index);
}

IndexType QVtkTableModelPolyData::indexType() const
{
    return IndexType::cells;
}

void QVtkTableModelPolyData::resetDisplayData()
{
    m_polyData = dynamic_cast<PolyDataObject *>(dataObject());

    if (m_polyData)
    {
        auto dataSet = m_polyData->dataSet();
        auto cellTypes = vtkSmartPointer<vtkCellTypes>::New();
        dataSet->GetCellTypes(cellTypes);

        if (cellTypes->GetNumberOfTypes() == 1 && dataSet->GetCellType(0) == VTK_TRIANGLE)
        {
            m_cellTypeName = "triangle";
        }
        else
        {
            m_cellTypeName = "cell";
        }
    }
    else
    {
        m_cellTypeName.clear();
    }
}
