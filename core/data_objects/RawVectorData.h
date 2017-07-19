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

#include <core/data_objects/DataObject.h>


class vtkFloatArray;


class CORE_API RawVectorData : public DataObject
{
public:
    RawVectorData(const QString & name, vtkFloatArray & dataArray);
    ~RawVectorData() override;

    /** Unsupported for this class, as the ctor requires a data array as 2nd parameter. */
    std::unique_ptr<DataObject> newInstance(const QString & name, vtkDataSet * dataSet) const override;

    bool is3D() const override;
    IndexType defaultAttributeLocation() const override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

    vtkFloatArray * dataArray();

protected:
    std::unique_ptr<QVtkTableModel> createTableModel() override;

private:
    vtkSmartPointer<vtkFloatArray> m_dataArray;

private:
    Q_DISABLE_COPY(RawVectorData)
};
