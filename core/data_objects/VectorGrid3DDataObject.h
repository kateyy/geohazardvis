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

#include <array>

#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <core/utility/DataExtent_fwd.h>


class vtkImageData;
class vtkAssignAttribute;


class CORE_API VectorGrid3DDataObject : public CoordinateTransformableDataObject
{
public:
    VectorGrid3DDataObject(const QString & name, vtkImageData & dataSet);
    ~VectorGrid3DDataObject() override;

    std::unique_ptr<DataObject> newInstance(const QString & name, vtkDataSet * dataSet) const override;

    bool is3D() const override;
    IndexType defaultAttributeLocation() const override;

    std::unique_ptr<RenderedData> createRendered() override;

    void addDataArray(vtkDataArray & dataArray) override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

    vtkImageData & imageData();
    const vtkImageData & imageData() const;

    /** index of first and last point on each axis (min/max per x, y, z) */
    ImageExtent extent();
    /** number of vector data components  */
    int numberOfComponents();
    /** scalar range for specified vector component */
    ValueRange<> scalarRange(int component);

protected:
    std::unique_ptr<QVtkTableModel> createTableModel() override;

    bool checkIfStructureChanged() override;

    vtkSmartPointer<vtkAlgorithm> createTransformPipeline(
        const CoordinateSystemSpecification & toSystem,
        vtkAlgorithmOutput * pipelineUpstream) const override;

private:
    std::array<int, 6> m_extent;

private:
    Q_DISABLE_COPY(VectorGrid3DDataObject)
};
