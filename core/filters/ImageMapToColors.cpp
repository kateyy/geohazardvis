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
