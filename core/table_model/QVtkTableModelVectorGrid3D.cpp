/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "QVtkTableModelVectorGrid3D.h"

#include <array>
#include <cassert>

#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkCell.h>

#include <core/types.h>
#include <core/data_objects/DataObject.h>


QVtkTableModelVectorGrid3D::QVtkTableModelVectorGrid3D(QObject * parent)
    : QVtkTableModel(parent)
    , m_gridData{ nullptr }
    , m_scalars{ nullptr }
{
}

QVtkTableModelVectorGrid3D::~QVtkTableModelVectorGrid3D() = default;

int QVtkTableModelVectorGrid3D::rowCount(const QModelIndex &/*parent*/) const
{
    if (!m_gridData)
    {
        return 0;
    }

    return static_cast<int>(m_gridData->GetNumberOfPoints());
}

int QVtkTableModelVectorGrid3D::columnCount(const QModelIndex &/*parent*/) const
{
    if (!m_gridData)
    {
        return 0;
    }


    const int columnCount = 1 + 3
        + (m_scalars ? m_scalars->GetNumberOfComponents() : 0);

    return columnCount;
    
}

QVariant QVtkTableModelVectorGrid3D::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !m_gridData)
    {
        return QVariant();
    }

    const vtkIdType pointId = index.row();

    if (index.column() == 0)
    {
        return pointId;
    }

    if (index.column() <= 3)
    {
        std::array<double, 3> point;
        m_gridData->GetPoint(pointId, point.data());
        return point[index.column() - 1];
    }

    const int component = index.column() - 4;
    assert(m_scalars && (m_scalars->GetNumberOfComponents() > component));
    std::array<double, 3> tuple;
    m_scalars->GetTuple(pointId, tuple.data()); 
    return tuple[component];
}

QVariant QVtkTableModelVectorGrid3D::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
    {
        return QVtkTableModel::headerData(section, orientation, role);
    }

    switch (section)
    {
    case 0: return "ID";
    case 1: return "point x";
    case 2: return "point y";
    case 3: return "point z";
    default:
        assert(m_scalars);
        const int component = section - 4;
        if (m_scalars->GetNumberOfComponents() == 1)
        {
            return "data";
        }
        if (m_scalars->GetNumberOfComponents() == 3)
        {
            return QString("vector ") + QChar('x' + component);
        }
        return "data " + QString::number(component + 1);
    }
}

bool QVtkTableModelVectorGrid3D::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole || index.column() < 4 || !m_gridData)
    {
        return false;
    }

    bool validValue;
    const double newValue = value.toDouble(&validValue);
    if (!validValue)
    {
        return false;
    }

    const vtkIdType tupleId = index.row();

    int numComponents = m_scalars->GetNumberOfComponents();
    int componentId = index.column() - 4;
    assert(componentId >= 0 && componentId < numComponents);


    std::vector<double> tuple(numComponents);

    m_scalars->GetTuple(tupleId, tuple.data());
    tuple[componentId] = newValue;
    m_scalars->SetTuple(tupleId, tuple.data());
    m_scalars->Modified();
    m_gridData->Modified();

    return false;
}

Qt::ItemFlags QVtkTableModelVectorGrid3D::flags(const QModelIndex &index) const
{
    if (index.column() > 3)
    {
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
    }

    return QAbstractItemModel::flags(index);
}

IndexType QVtkTableModelVectorGrid3D::indexType() const
{
    return IndexType::points;
}

void QVtkTableModelVectorGrid3D::resetDisplayData()
{
    m_gridData = dataObject() ? dataObject()->dataSet() : nullptr;

    if (m_gridData)
    {
        m_scalars = m_gridData->GetPointData()->GetScalars();
    }
}
