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

#include <vtkDataSetAlgorithm.h>

#include <core/CoordinateSystems.h>


/**
 * Geographic transformations and metric unit conversions.
 *
 * This filter transforms data sets between geographic, projected (metric), and local coordinates.
 * It currently supports geographic coordinates (longitude/latitude) in WGS 84 and projections to
 * single UTM grid cells. A geographic reference point specified in the input data is required for
 * all coordinate transformations. For projections from/to UTM, the filter assumes the UTM zone in
 * which the geographic reference point is located. When converting to local coordinates, the input
 * coordinates are first projected to UTM (if required). These metric coordinates are than shifted
 * in a way so that the geographic reference point would be located at (0, 0) in local coordinates.
 *
 * **Units and conversions.** Metric coordinates (projected global/local), must be specified in
 * meters, possibly with any metric prefix. Conversions between different metric units can be done
 * with this filter. Geographic coordinates are always assumed to be in degrees.
 *
 * **Elevations.** This filter only converts the unit of elevations when transforming between
 * metric coordinate system. When transforming from/to geographic coordinates no modifications are
 * applied.
 *
 * **Input/Output.** This filter supports subclasses of vtkPointSet and vtkImageData as input, and
 * will output a data set of the same data type as the input. Only point coordinates will be
 * modified. All topology and attributes are preserved.
 *
 * @see ReferencedCoordinateSystemSpecification, SetCoordinateSystemInformationFilter
 */
class CORE_API GeographicTransformationFilter : public vtkDataSetAlgorithm
{
public:
    vtkTypeMacro(GeographicTransformationFilter, vtkDataSetAlgorithm);
    static GeographicTransformationFilter * New();

    static bool IsTransformationSupported(
        const ReferencedCoordinateSystemSpecification & sourceSpec,
        const CoordinateSystemSpecification & targetSpec);

    vtkGetMacro(TargetCoordinateSystem, const CoordinateSystemSpecification &);
    vtkSetMacro(TargetCoordinateSystem, CoordinateSystemSpecification);

    /**
     * For vtkPointSet subclasses: modify the input point coordinate array directly.
     * Warning: this will overwrite pipeline upstream data!
     */
    vtkGetMacro(OperateInPlace, bool);
    vtkSetMacro(OperateInPlace, bool);
    vtkBooleanMacro(OperateInPlace, bool);

protected:
    GeographicTransformationFilter();
    ~GeographicTransformationFilter() override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int FillInputPortInformation(int port, vtkInformation * info) override;

private:
    ReferencedCoordinateSystemSpecification SourceCoordinateSystem;
    CoordinateSystemSpecification TargetCoordinateSystem;
    ReferencedCoordinateSystemSpecification ReferencedTargetSpec;
    bool OperateInPlace;

private:
    GeographicTransformationFilter(const GeographicTransformationFilter &) = delete;
    void operator=(const GeographicTransformationFilter &) = delete;
};
