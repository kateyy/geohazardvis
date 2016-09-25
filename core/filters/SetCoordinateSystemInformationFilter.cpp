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
    auto outData = outInfo->Get(vtkDataObject::DATA_OBJECT());

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
