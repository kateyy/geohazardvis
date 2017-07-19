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


class vtkDataArray;


class CORE_API NoiseImageSource : public vtkImageAlgorithm
{
public:
    static NoiseImageSource * New();
    vtkTypeMacro(NoiseImageSource, vtkImageAlgorithm);

    enum
    {
        CpuUniformDistribution,
        GpuPerlinNoise
    };
    vtkSetMacro(NoiseSource, int);
    vtkGetMacro(NoiseSource, int);

    vtkSetStringMacro(ScalarsName);
    vtkGetStringMacro(ScalarsName);

    vtkSetClampMacro(NumberOfComponents, int, 1, VTK_INT_MAX);
    vtkGetMacro(NumberOfComponents, int);

    vtkSetVector2Macro(ValueRange, double);
    vtkGetVector2Macro(ValueRange, double);

    vtkSetMacro(Seed, unsigned long);
    vtkGetMacro(Seed, unsigned long);

    virtual void SetExtent(int extent[6]);
    virtual void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);

    vtkSetVector3Macro(Origin, double);
    vtkGetVector3Macro(Origin, double);

    vtkSetVector3Macro(Spacing, double);
    vtkGetVector3Macro(Spacing, double);

    vtkGetVector6Macro(Extent, int);

protected:
    NoiseImageSource();
    ~NoiseImageSource() override;

    vtkIdType GetNumberOfTuples() const;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    NoiseImageSource(const NoiseImageSource&);  // Not implemented.
    void operator=(const NoiseImageSource&);  // Not implemented.

    int ExecuteCPUUniformDist(vtkDataArray * data);
    int ExecuteGpuPerlin(vtkDataArray * data);

private:
    char * ScalarsName;
    int NumberOfComponents;
    unsigned long Seed;

    int NoiseSource;
    int Extent[6];
    double Origin[3];
    double Spacing[3];
    double ValueRange[2];
};
