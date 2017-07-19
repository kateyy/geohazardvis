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

#include "QVtkTableModelPointCloudData.h"

#include <cassert>

#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkPointData.h>

#include <core/types.h>
#include <core/data_objects/GenericPolyDataObject.h>
#include <core/utility/vtkvarianthelper.h>


QVtkTableModelPointCloudData::QVtkTableModelPointCloudData(QObject * parent)
    : QVtkTableModel(parent)
    , m_data{ nullptr }
{
}

QVtkTableModelPointCloudData::~QVtkTableModelPointCloudData() = default;

int QVtkTableModelPointCloudData::rowCount(const QModelIndex &/*parent*/) const
{
    if (!m_data)
    {
        return 0;
    }

    return static_cast<int>(m_data->numberOfPoints());
}

int QVtkTableModelPointCloudData::columnCount(const QModelIndex & /*parent*/) const
{
    auto s = scalars();
    const int scalarColumns = s ? s->GetNumberOfComponents() : 0;
    return 4 + scalarColumns;
}

QVariant QVtkTableModelPointCloudData::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !m_data)
    {
        return{};
    }

    const auto pointId = static_cast<vtkIdType>(index.row());

    switch (index.column())
    {
    case 0:
        return pointId;
    case 1:
    case 2:
    case 3:
    {
        const int component = index.column() - 1;
        assert(component >= 0 && component < 3);
        bool okay;
        const auto value = m_data->pointCoordinateComponent(pointId, component, &okay);
        if (okay)
        {
            return value;
        }
        return{};
    }
    }

    const int scalarComponent = index.column() - 4;
    auto s = scalars();
    if (!s || s->GetNumberOfComponents() <= scalarComponent)
    {
        return{};
    }
    
    return vtkVariantToQVariant(
        s->GetVariantValue(pointId * s->GetNumberOfComponents() + scalarComponent));
}

QVariant QVtkTableModelPointCloudData::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
    {
        return QVtkTableModel::headerData(section, orientation, role);
    }

    switch (section)
    {
    case 0: return "Point ID";
    case 1: return "x";
    case 2: return "y";
    case 3: return "z";
    }

    const int scalarComponent = section - 4;
    auto s = scalars();
    const auto numComponents = s->GetNumberOfComponents();
    if (!s || numComponents <= scalarComponent)
    {
        return{};
    }

    QString componentName;
    if (numComponents > 0 && numComponents <= 3)
    {
        componentName = QChar('x' + static_cast<char>(scalarComponent));
    }
    else if (numComponents > 3)
    {
        componentName = "(" + QString::number(scalarComponent) + ")";
    }
    
    if (scalarComponent == 0)
    {
        auto name = QString::fromUtf8(s->GetName());
        if (name.isEmpty())
        {
            name = "Scalars";
        }
        if (!componentName.isEmpty())
        {
            name += ": " + componentName;
        }
        return name;
    }
    return componentName;
}

bool QVtkTableModelPointCloudData::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole || index.column() < 1 || !m_data)
    {
        return false;
    }

    bool ok;
    const double newValue = value.toDouble(&ok);
    if (!ok)
    {
        return false;
    }

    const auto pointId = static_cast<vtkIdType>(index.row());
    const int component = index.column() - 1;
    assert(component >= 0 && component < 3);

    return m_data->setPointCoordinateComponent(pointId, component, newValue);
}

Qt::ItemFlags QVtkTableModelPointCloudData::flags(const QModelIndex &index) const
{
    if (index.column() > 1)
    {
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
    }

    return QAbstractItemModel::flags(index);
}

IndexType QVtkTableModelPointCloudData::indexType() const
{
    return IndexType::points;
}

void QVtkTableModelPointCloudData::resetDisplayData()
{
    m_data = dynamic_cast<GenericPolyDataObject *>(dataObject());
    if (!m_data)
    {
        m_lastScalars = nullptr;
        return;
    }
    
    auto dataSet = m_data ? m_data->processedOutputDataSet() : static_cast<vtkDataSet *>(nullptr);
    if (!dataSet)
    {
        m_lastScalars = nullptr;
    }
    auto scalars = dataSet->GetPointData()->GetScalars();
    if (!scalars || scalars->GetNumberOfTuples() != dataSet->GetNumberOfPoints())
    {
        m_lastScalars = nullptr;
    }
    m_lastScalars = scalars;

    addDataObjectConnection(connect(m_data, &DataObject::attributeArraysChanged,
        this, &QVtkTableModelPointCloudData::rebuild));
}

vtkDataArray * QVtkTableModelPointCloudData::scalars() const
{
    return m_lastScalars;
}
