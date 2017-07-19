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

#include <vtkThreadedImageAlgorithm.h>

#include <core/core_api.h>


class CORE_API DEMImageNormals : public vtkThreadedImageAlgorithm
{
public:
    vtkTypeMacro(DEMImageNormals, vtkThreadedImageAlgorithm);
    static DEMImageNormals * New();

    vtkGetMacro(CoordinatesUnitScale, double);
    vtkSetMacro(CoordinatesUnitScale, double);

    vtkGetMacro(ElevationUnitScale, double);
    vtkSetMacro(ElevationUnitScale, double);

protected:
    DEMImageNormals();
    ~DEMImageNormals() override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;
    
    void CopyAttributeData(vtkImageData * in, vtkImageData * out,
        vtkInformationVector ** inputVector) override;

    void ThreadedRequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector,
        vtkImageData *** inData, vtkImageData ** outData,
        int outExt[6], int id) override;

private:
    double CoordinatesUnitScale;
    double ElevationUnitScale;

private:
    DEMImageNormals(const DEMImageNormals &) = delete;
    void operator=(const DEMImageNormals &) = delete;
};
