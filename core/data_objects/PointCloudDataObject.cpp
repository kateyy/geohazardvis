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

#include "PointCloudDataObject.h"

#include <vtkDataArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>

#include <core/types.h>
#include <core/rendered_data/RenderedPointCloudData.h>
#include <core/table_model/QVtkTableModelPointCloudData.h>


PointCloudDataObject::PointCloudDataObject(const QString & name, vtkPolyData & dataSet)
    : GenericPolyDataObject(name, dataSet)
{
}

PointCloudDataObject::~PointCloudDataObject() = default;

std::unique_ptr<DataObject> PointCloudDataObject::newInstance(const QString & name, vtkDataSet * dataSet) const
{
    if (auto poly = vtkPolyData::SafeDownCast(dataSet))
    {
        return std::make_unique<PointCloudDataObject>(name, *poly);
    }
    return{};
}

IndexType PointCloudDataObject::defaultAttributeLocation() const
{
    return IndexType::points;
}

std::unique_ptr<RenderedData> PointCloudDataObject::createRendered()
{
    return std::make_unique<RenderedPointCloudData>(*this);
}

void PointCloudDataObject::addDataArray(vtkDataArray & dataArray)
{
    dataSet()->GetPointData()->AddArray(&dataArray);
}

const QString & PointCloudDataObject::dataTypeName() const
{
    return dataTypeName_s();
}

const QString & PointCloudDataObject::dataTypeName_s()
{
    static const QString name{ "Point Cloud" };

    return name;
}

std::unique_ptr<QVtkTableModel> PointCloudDataObject::createTableModel()
{
    auto model = std::make_unique<QVtkTableModelPointCloudData>();
    model->setDataObject(this);

    return std::move(model);
}
