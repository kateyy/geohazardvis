#include <core/filters/TransformTextureCoords.h>

#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>


vtkStandardNewMacro(TransformTextureCoords);


int TransformTextureCoords::RequestData(
  vtkInformation * request,
  vtkInformationVector ** inputVector,
  vtkInformationVector * outputVector)
{
    int result = Superclass::RequestData(request, inputVector, outputVector);

    if (!result)
        return 0;

    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    vtkDataSet *input = vtkDataSet::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkDataSet *output = vtkDataSet::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT()));

    // copy attributes that got lost in Superclass request

    output->GetCellData()->PassData(input->GetCellData());
    output->GetFieldData()->PassData(input->GetFieldData());

    return 1;
}
