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

#include "ExtractTimeStep.h"

#include <algorithm>
#include <vector>

#include <vtkDataObject.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkObjectFactory.h>


vtkStandardNewMacro(ExtractTimeStep);


ExtractTimeStep::ExtractTimeStep()
    : Superclass()
    , TimeStep{ 0.0 }
{
}

ExtractTimeStep::~ExtractTimeStep() = default;

int ExtractTimeStep::RequestInformation(
    vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    if (!Superclass::RequestInformation(request, inputVector, outputVector))
    {
        return 0;
    }

    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);

    bool timeStepIsValid = false;
    do
    {
        if (!inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
        {
            break;
        }
        const auto count = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
        std::vector<double> timeSteps(count);
        inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeSteps.data());
        const auto it = std::find(timeSteps.begin(), timeSteps.end(), this->TimeStep);
        if (it == timeSteps.end())
        {
            break;
        }
        timeStepIsValid = true;
    } while (false);

    if (!timeStepIsValid)
    {
        inInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
        return 1;
    }

    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->TimeStep);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->TimeStep, 1);
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

    return 1;
}

int ExtractTimeStep::RequestData(
    vtkInformation * /*request*/,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inData = vtkDataObject::GetData(inputVector[0], 0);
    auto outData = vtkDataObject::GetData(outputVector, 0);

    if (inData && outData)
    {
        outData->ShallowCopy(inData);
    }

    return 1;
}
