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

#pragma once

#include <vtkSmartPointer.h>

#include <core/data_objects/GenericPolyDataObject.h>


class CORE_API PointCloudDataObject : public GenericPolyDataObject
{
public:
    PointCloudDataObject(const QString & name, vtkPolyData & dataSet);
    ~PointCloudDataObject() override;

    std::unique_ptr<DataObject> newInstance(const QString & name, vtkDataSet * dataSet) const override;

    IndexType defaultAttributeLocation() const override;

    void addDataArray(vtkDataArray & dataArray) override;

    std::unique_ptr<RenderedData> createRendered() override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

protected:
    std::unique_ptr<QVtkTableModel> createTableModel() override;

private:
    Q_DISABLE_COPY(PointCloudDataObject)
};
