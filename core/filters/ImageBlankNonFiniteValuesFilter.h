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

#include <vtkImageAlgorithm.h>

#include <core/core_api.h>


/**
 * Convert a vtkImageData to its subclass vtkUniformGrid and blank all input points with non-finite
 * scalar values (NaN or positive/negative infinity).
 *
 * As an example, this filter helps to correctly handle invalid values in grid data that is used as
 * source in vtkProbeFilter. The probe filter does not preserve non-finite values in some
 * situations and replaces them with garbage. (See implementation of vtkMath::Max, not properly
 * handling NaNs).
 * With a vtkUniformGrid, invalid values can be identified using the data set's point ghost array.
 */
class CORE_API ImageBlankNonFiniteValuesFilter : public vtkImageAlgorithm
{
public:
    vtkTypeMacro(ImageBlankNonFiniteValuesFilter, vtkImageAlgorithm);
    static ImageBlankNonFiniteValuesFilter * New();

protected:
    ImageBlankNonFiniteValuesFilter();
    ~ImageBlankNonFiniteValuesFilter() override;

    int FillOutputPortInformation(int port, vtkInformation * info) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    ImageBlankNonFiniteValuesFilter(const ImageBlankNonFiniteValuesFilter &) = delete;
    void operator=(const ImageBlankNonFiniteValuesFilter &) = delete;
};
