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

#include "AssignPointAttributeToCoordinatesFilter.h"

#include <cassert>

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPointSet.h>


vtkStandardNewMacro(AssignPointAttributeToCoordinatesFilter);


AssignPointAttributeToCoordinatesFilter::AssignPointAttributeToCoordinatesFilter()
    : Superclass()
    , CurrentCoordinatesAsScalars{ false }
{
}

AssignPointAttributeToCoordinatesFilter::~AssignPointAttributeToCoordinatesFilter() = default;

int AssignPointAttributeToCoordinatesFilter::RequestData(
    vtkInformation * /*request*/,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inData = vtkPointSet::SafeDownCast(inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
    auto outData = vtkPointSet::SafeDownCast(outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));

    auto previousPointCoords = inData->GetPoints()->GetData();
    vtkDataArray * pointsToAssign = nullptr;

    if (!this->AttributeArrayToAssign.empty())
    {
        auto newPoints = inData->GetPointData()->GetArray(this->AttributeArrayToAssign.c_str());
        if (!newPoints)
        {
            vtkErrorMacro("Array to assign not found in input data: " + this->AttributeArrayToAssign);
            return 0;
        }

        if (newPoints->GetNumberOfComponents() != 3
            || newPoints->GetNumberOfTuples() != inData->GetNumberOfPoints())
        {
            vtkErrorMacro("Component/Tuple count mismatching in selected data array: " + this->AttributeArrayToAssign);
            return 0;
        }
        pointsToAssign = newPoints;
    }

    outData->ShallowCopy(inData);

    if (pointsToAssign)
    {
        vtkNew<vtkPoints> newPoints;
        newPoints->SetData(pointsToAssign);
        outData->SetPoints(newPoints.Get());
    }

    // pass current point coordinates as point attribute
    if (previousPointCoords)
    {
        outData->GetPointData()->AddArray(previousPointCoords);
    }

    if (auto currentCoords = pointsToAssign ? pointsToAssign : previousPointCoords)
    {
        outData->GetPointData()->SetActiveScalars(currentCoords->GetName());
    }

    return 1;
}
