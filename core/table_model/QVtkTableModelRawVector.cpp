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

#include "QVtkTableModelRawVector.h"

#include <array>
#include <cassert>

#include <vtkFloatArray.h>

#include <core/data_objects/RawVectorData.h>


QVtkTableModelRawVector::QVtkTableModelRawVector(QObject * parent)
    : QVtkTableModel(parent)
{
}

QVtkTableModelRawVector::~QVtkTableModelRawVector() = default;

int QVtkTableModelRawVector::rowCount(const QModelIndex &/*parent*/) const
{
    if (!m_data)
    {
        return 0;
    }

    return static_cast<int>(m_data->GetNumberOfTuples());
}

int QVtkTableModelRawVector::columnCount(const QModelIndex &/*parent*/) const
{
    if (!m_data)
    {
        return 0;
    }

    return 1 + m_data->GetNumberOfComponents();
}

QVariant QVtkTableModelRawVector::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !m_data)
    {
        return QVariant();
    }

    const vtkIdType tupleId = index.row();

    if (index.column() == 0)
    {
        return tupleId;
    }

    const vtkIdType component = index.column() - 1;
    assert(component < m_data->GetNumberOfComponents());

    std::array<double, 3> tuple;
    m_data->GetTuple(tupleId, tuple.data());
    return tuple[component];
}

QVariant QVtkTableModelRawVector::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal || !m_data)
    {
        return QVtkTableModel::headerData(section, orientation, role);
    }

    if (section == 0)
    {
        return "ID";
    }

    const vtkIdType components = m_data->GetNumberOfComponents();

    switch (components)
    {
    case 1:
        return "value";
    case 2:
    case 3:
        return QChar('x' + section - 1);
    default:
        return section - 1;
    }
}

bool QVtkTableModelRawVector::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole || index.column() == 0 || !m_data)
    {
        return false;
    }

    bool ok;
    const double f_value = value.toDouble(&ok);
    if (!ok)
    {
        return false;
    }

    const vtkIdType tupleId = index.row(), component = index.column() - 1;
    int components = m_data->GetNumberOfComponents();

    assert(component < components && tupleId < m_data->GetNumberOfTuples());

    std::vector<double> tupleData(components);

    m_data->GetTuple(tupleId, tupleData.data());
    tupleData[component] = f_value;
    m_data->SetTuple(tupleId, tupleData.data());

    m_data->Modified();

    return true;
}

Qt::ItemFlags QVtkTableModelRawVector::flags(const QModelIndex &index) const
{
    if (index.column() > 0)
    {
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
    }

    return QAbstractItemModel::flags(index);
}

IndexType QVtkTableModelRawVector::indexType() const
{
    return IndexType();
}

void QVtkTableModelRawVector::resetDisplayData()
{
    if (auto dataVector = dynamic_cast<RawVectorData *>(dataObject()))
    {
        m_data = dataVector->dataArray();
    }
    else
    {
        m_data = nullptr;
    }
}
