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

#include <vector>

#include <vtkSmartPointer.h>

#include <core/CoordinateSystems_fwd.h>


class vtkDoubleArray;
class vtkPolyData;
class vtkVector3d;
class GeographicTransformationFilter;
class SetCoordinateSystemInformationFilter;



/** Convenience class that simplifies transformation of a raw point vector. */
class CORE_API GeographicTransformationUtil
{
public:
    GeographicTransformationUtil();
    virtual ~GeographicTransformationUtil();

    void setSourceSystem(const ReferencedCoordinateSystemSpecification & sourceSpec);
    const ReferencedCoordinateSystemSpecification & sourceSystem() const;
    void setTargetSystem(const CoordinateSystemSpecification & targetSpec);
    const CoordinateSystemSpecification & targetSystem() const;

    /** Check whether the transformation from current source to target system is supported. */
    bool isTransformationSupported() const;

    /**
    * @param points Vector of point coordinates to be transformed in place.
    * @return true only if the transformation succeeded. It should succeed if
    *  IsConversionSupported() returns true for the coordinate systems and the source point
    *  coordinates are in sensible range.
    */
    bool transformPoints(std::vector<vtkVector3d> & points);
    bool transformPoints(vtkVector3d * points, size_t numPoints);

    /**
    * Variant of TransformPoints that transforms a single point.
    * @param successPtr Pass a boolean here if you want to verify that the conversion succeeded.
    *  It should succeed if IsConversionSupported() returns true for the coordinate systems
    *  and the source point coordinates are in sensible range.
    * @return The transformed point.
    */
    vtkVector3d transformPoint(const vtkVector3d & sourcePoint, bool * successPtr = nullptr);

private:
    void setPoints(vtkVector3d * points, vtkIdType numPoints);
    void releasePoints();

private:
    vtkSmartPointer<vtkDoubleArray> m_coordinates;
    vtkSmartPointer<vtkPolyData> m_dataSet;
    vtkSmartPointer<SetCoordinateSystemInformationFilter> m_spec;
    vtkSmartPointer<GeographicTransformationFilter> m_transform;
};
