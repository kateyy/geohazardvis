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

#include "AttributeArrayModifiedListener.h"

#include <vtkDataArray.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>


vtkStandardNewMacro(AttributeArrayModifiedListener);


void AttributeArrayModifiedListener::SetAttributeLocation(IndexType location)
{
    if (location == this->AttributeLocation)
    {
        return;
    }

    this->AttributeLocation = location;
    this->LastAttributeMTime = 0;
}

AttributeArrayModifiedListener::AttributeArrayModifiedListener()
    : Superclass()
    , AttributeLocation{ IndexType::points }
    , LastAttributeMTime{ 0 }
{
}

AttributeArrayModifiedListener::~AttributeArrayModifiedListener() = default;

int AttributeArrayModifiedListener::RequestData(
    vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    if (!Superclass::RequestData(request, inputVector, outputVector))
    {
        return 0;
    }

    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto inData = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

    if (this->AttributeLocation != IndexType::points
        && this->AttributeLocation != IndexType::cells)
    {
        vtkWarningMacro(<< "Invalid attribute location selected.");
        // Just warn, don't fail the whole pipeline.
        return 1;
    }

    auto attributes = this->AttributeLocation == IndexType::points
        ? static_cast<vtkDataSetAttributes *>(inData->GetPointData())
        : static_cast<vtkDataSetAttributes *>(inData->GetCellData());

    auto scalars = attributes->GetScalars();
    if (!scalars)
    {
        vtkWarningMacro(<< "No scalars found in selected attribute location.");
        return 1;
    }

    const auto currentMTime = scalars->GetMTime();

    if (this->LastAttributeMTime == 0)
    {
        this->LastAttributeMTime = currentMTime;
        return 1;
    }

    if (this->LastAttributeMTime == currentMTime)
    {
        return 1;
    }

    this->LastAttributeMTime = currentMTime;

    emit attributeModified(scalars);

    return 1;
}
