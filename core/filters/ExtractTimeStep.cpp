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
