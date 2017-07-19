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

#include <vtkSelectionAlgorithm.h>
#include <vtkVector.h>

#include <core/core_api.h>


class vtkPolyData;


/**
* From an input set of points, extract points that are located within the range of a line segment.
* The data set and the line are assumed to be placed on the XY-plane.
* 
* This filter requires an input line segment specified by StartPoint and EndPoint.
* An input point is considered in range, if:
*     * Its distance to the line is less than the double approximated grid spacing.
*
* The input points are assumed to be approximately aligned on an (incomplete) grid.
*
* Two outputs are produced:
*     Port 0: vtkSelection containing relevant cell indices
*     Port 1: vtkPolyData containing selected points with additional attributes assigned
*/
class CORE_API LineOnPointsSelector2D : public vtkSelectionAlgorithm
{
public:
    vtkTypeMacro(LineOnPointsSelector2D, vtkSelectionAlgorithm);

    static LineOnPointsSelector2D * New();

    vtkPolyData * GetExtractedPoints();

    vtkGetMacro(StartPoint, vtkVector2d);
    vtkSetMacro(StartPoint, vtkVector2d);

    vtkGetMacro(EndPoint, vtkVector2d);
    vtkSetMacro(EndPoint, vtkVector2d);


    enum SortMode
    {
        SortIndices,
        SortPoints,
        SortNone
    };

    /** Set whether output points (port 1) are ordered by their projected position on the extraction line.
      * SortIndices: Point indices are sorted. To extract points (and point attributes) in the
      *     correct order, you have to lookup them using the ordering specified by the indices.
      * SortPoints: Points (and point attributes) are sorted. In this case, the point indices are
      *     simply in increasing order.
      * SortNone: Don't apply any sorting. Extracted points remain in the same order as their
      *     respective points in the input data.
    */
    vtkGetMacro(Sorting, SortMode);
    vtkSetClampMacro(Sorting, SortMode, SortIndices, SortNone);
    vtkBooleanMacro(Sorting, SortMode);

    vtkGetMacro(PassPositionOnLine, bool);
    vtkSetMacro(PassPositionOnLine, bool);
    vtkBooleanMacro(PassPositionOnLine, bool);

    vtkGetMacro(PassDistanceToLine, bool);
    vtkSetMacro(PassDistanceToLine, bool);
    vtkBooleanMacro(PassDistanceToLine, bool);

protected:
    LineOnPointsSelector2D();
    ~LineOnPointsSelector2D() override;

    int FillInputPortInformation(int port, vtkInformation * info) override;
    int FillOutputPortInformation(int port, vtkInformation * info) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    vtkVector2d StartPoint;
    vtkVector2d EndPoint;
    SortMode Sorting;
    bool PassPositionOnLine;
    bool PassDistanceToLine;
    vtkMTimeType InputPointsMTime;
    double ApproxGridSpacing;
    double GridSpacingStandardDeviation;

public:
    LineOnPointsSelector2D(const LineOnPointsSelector2D &) = delete;
    void operator=(const LineOnPointsSelector2D &) = delete;
};
