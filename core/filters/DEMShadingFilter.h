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

class CORE_API DEMShadingFilter : public vtkImageAlgorithm
{
public:
    static DEMShadingFilter * New();
    vtkTypeMacro(DEMShadingFilter, vtkImageAlgorithm);

    vtkGetMacro(Diffuse, double);
    vtkSetClampMacro(Diffuse, double, 0.0, 1.0);

    vtkGetMacro(Ambient, double);
    vtkSetClampMacro(Ambient, double, 0.0, 1.0);

protected:
    DEMShadingFilter();
    ~DEMShadingFilter() override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    double Diffuse;
    double Ambient;

private:
    DEMShadingFilter(const DEMShadingFilter &) = delete;
    void operator=(const DEMShadingFilter &) = delete;
};
