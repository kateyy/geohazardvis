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

#include <gui/gui_api.h>


class vtkDataSet;
class vtkImageData;
class vtkPolyData;
class vtkRenderer;

class AbstractVisualizedData;
class DataObject;
enum class IndexType;


class GUI_API CameraDolly
{
public:
    void setRenderer(vtkRenderer * renderer);
    vtkRenderer * renderer() const;

    /** Move the camera over time to look at the specified primitive in the visualization. */
    void moveTo(AbstractVisualizedData & visualization, vtkIdType index, IndexType indexType, bool overTime = true);
    void moveTo(DataObject & dataObject, vtkIdType index, IndexType indexType, bool overTime = true);
    void moveTo(vtkDataSet & dataSet, vtkIdType index, IndexType indexType, bool overTime = true);


    void moveToPoly(vtkPolyData & poly, vtkIdType index, IndexType indexType, bool overTime);
    void moveToImage(vtkImageData & image, vtkIdType index, IndexType indexType, bool overTime);

private:
    vtkSmartPointer<vtkRenderer> m_renderer;
};
