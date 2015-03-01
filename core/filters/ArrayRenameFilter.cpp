#include "ArrayRenameFilter.h"

#include <cassert>

#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>



vtkStandardNewMacro(ArrayRenameFilter);


ArrayRenameFilter::ArrayRenameFilter()
    : ScalarsName(nullptr)
{
}

int ArrayRenameFilter::RequestData(
    vtkInformation *,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    vtkInformation * inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation * outInfo = outputVector->GetInformationObject(0);

    vtkDataObject * input = inInfo->Get(vtkDataObject::DATA_OBJECT());
    vtkDataObject * output = outInfo->Get(vtkDataObject::DATA_OBJECT());

    vtkDataSet * dsInput = vtkDataSet::SafeDownCast(input);
    vtkDataSet * dsOutput = vtkDataSet::SafeDownCast(output);
    assert(dsInput && dsOutput);

    dsOutput->CopyStructure(dsInput);

    if (dsOutput->GetFieldData() && dsInput->GetFieldData())
    {
        dsOutput->GetFieldData()->PassData(dsInput->GetFieldData());
    }
    dsOutput->GetPointData()->PassData(dsInput->GetPointData());
    dsOutput->GetCellData()->PassData(dsInput->GetCellData());


    vtkDataArray * scalars = dsOutput->GetPointData()->GetScalars();
    if (!scalars)
        return 0;
        
    scalars->SetName(ScalarsName);

    return 1;
}
