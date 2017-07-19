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

#include "SetCoordinateSystemInformationFilter.h"

#include <vtkDataObject.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>


vtkStandardNewMacro(SetCoordinateSystemInformationFilter)


SetCoordinateSystemInformationFilter::SetCoordinateSystemInformationFilter()
    : Superclass()
{
}

SetCoordinateSystemInformationFilter::~SetCoordinateSystemInformationFilter() = default;

int SetCoordinateSystemInformationFilter::RequestInformation(vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    if (!Superclass::RequestInformation(request, inputVector, outputVector))
    {
        return 0;
    }

    auto outInfo = outputVector->GetInformationObject(0);

    this->CoordinateSystemSpec.writeToInformation(*outInfo);

    return 1;
}

int SetCoordinateSystemInformationFilter::RequestData(vtkInformation * /*request*/,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);
    auto input = inInfo->Get(vtkDataObject::DATA_OBJECT());
    auto output = outInfo->Get(vtkDataObject::DATA_OBJECT());

    output->ShallowCopy(input);

    this->CoordinateSystemSpec.writeToFieldData(*output->GetFieldData());

    return 1;
}
