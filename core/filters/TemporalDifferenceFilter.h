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

#include <vtkDataSetAlgorithm.h>

#include <core/core_api.h>


/**
 * Compute the difference between scalars at two time steps provided by an upstream algorithm.
 *
 * The scalar difference is computed from the first to the second time step, thus: d = t1 - t0
 */
class CORE_API TemporalDifferenceFilter : public vtkDataSetAlgorithm
{
public:
    vtkTypeMacro(TemporalDifferenceFilter, vtkDataSetAlgorithm);
    static TemporalDifferenceFilter * New();

    void PrintSelf(std::ostream & os, vtkIndent indent) override;

    vtkGetMacro(TimeStep0, double);
    vtkSetMacro(TimeStep0, double);

    vtkGetMacro(TimeStep1, double);
    vtkSetMacro(TimeStep1, double);

protected:
    TemporalDifferenceFilter();
    ~TemporalDifferenceFilter() override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestUpdateExtent(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    bool InitProcess(vtkDataSet & input, vtkDataSet & output);
    bool ComputeDifferences(vtkDataSet & input, vtkDataSet & output);

private:
    double TimeStep0;
    double TimeStep1;

    // Used when iterating the pipeline to keep track of which time step we are on.
    enum class ProcessStep
    {
        init, difference, done
    };
    ProcessStep CurrentProcessStep;

private:
    TemporalDifferenceFilter(const TemporalDifferenceFilter &) = delete;
    void operator=(const TemporalDifferenceFilter &) = delete;
};
