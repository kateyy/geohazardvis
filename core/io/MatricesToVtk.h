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

#include <memory>

#include <core/core_api.h>
#include <core/io/types.h>


class QString;
class vtkDataArray;
class vtkImageData;
class vtkPolyData;
class DataObject;


class CORE_API MatricesToVtk
{
public:
    static std::unique_ptr<DataObject> loadIndexedTriangles(const QString & name, const std::vector<io::ReadDataSet> & datasets);
    static std::unique_ptr<DataObject> loadGrid2D(const QString & name, const std::vector<io::ReadDataSet> & datasets);
    static std::unique_ptr<DataObject> loadGrid3D(const QString & name, const std::vector<io::ReadDataSet> & datasets);

    static std::unique_ptr<DataObject> readRawFile(const QString & fileName);

    static vtkSmartPointer<vtkImageData> parseGrid2D(const io::InputVector & inputData, int vtk_dataType = VTK_FLOAT);
    static vtkSmartPointer<vtkPolyData> parsePoints(const io::InputVector & inputData, size_t firstColumn,
        int vtk_dataType = VTK_FLOAT);
    static vtkSmartPointer<vtkPolyData> parseIndexedTriangles(
        const io::InputVector & inputVertexData, size_t vertexIndexColumn, size_t firstVertexColumn,
        const io::InputVector & inputIndexData, size_t firstIndexColumn,
        int vtk_dataType = VTK_FLOAT);
    static vtkSmartPointer<vtkDataArray> parseFloatVector(
        const io::InputVector & inputData, const QString & arrayName, size_t firstColumn, size_t lastColumn,
        int vtk_dataType = VTK_FLOAT);

private:
    MatricesToVtk() = delete;
    ~MatricesToVtk() = delete;
};
