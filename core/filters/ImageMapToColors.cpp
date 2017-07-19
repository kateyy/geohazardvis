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

#include "ImageMapToColors.h"

#include <cassert>

#include <vtkDataSet.h>
#include <vtkObjectFactory.h>


vtkStandardNewMacro(ImageMapToColors);


ImageMapToColors::ImageMapToColors()
    : Superclass()
{
}

ImageMapToColors::~ImageMapToColors() = default;

int ImageMapToColors::RequestData(
    vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inData = vtkDataSet::SafeDownCast(vtkDataObject::GetData(inputVector[0]));
    auto outData = vtkDataSet::SafeDownCast(vtkDataObject::GetData(outputVector));
    assert(inData && outData);

    outData->CopyStructure(inData);

    return this->Superclass::RequestData(request, inputVector, outputVector);
}
