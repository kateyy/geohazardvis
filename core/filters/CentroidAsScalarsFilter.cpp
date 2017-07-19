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

#include "CentroidAsScalarsFilter.h"

#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkCellData.h>


vtkStandardNewMacro(CentroidAsScalarsFilter);


CentroidAsScalarsFilter::CentroidAsScalarsFilter()
    : Superclass()
{
    SetNumberOfInputPorts(2);
    SetNumberOfOutputPorts(1);
}

CentroidAsScalarsFilter::~CentroidAsScalarsFilter() = default;

int CentroidAsScalarsFilter::RequestData(vtkInformation * /*request*/,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto input = vtkPolyData::GetData(inputVector[0], 0);
    auto centroidsPoly = vtkPolyData::GetData(inputVector[1], 0);
    auto centroids = centroidsPoly->GetPoints()->GetData();
    auto output = vtkPolyData::GetData(outputVector, 0);

    // Check the size of the input.
    const vtkIdType numCells = input->GetNumberOfCells();
    if (numCells <= 0 || !centroids)
    {
        vtkDebugMacro("No input!");
        return 1;
    }
    if (numCells != centroids->GetNumberOfTuples())
    {
        vtkDebugMacro("Number of cells doesn't match number of centroid points!");
        return 1;
    }

    // Copy all the input geometry and data to the output.
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());

    // Add the new scalars array to the output.
    output->GetCellData()->SetScalars(centroids);

    return 1;
}
