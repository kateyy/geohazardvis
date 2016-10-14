#include "AssignPointAttributeToCoordinatesFilter.h"

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPointSet.h>


vtkStandardNewMacro(AssignPointAttributeToCoordinatesFilter);


AssignPointAttributeToCoordinatesFilter::AssignPointAttributeToCoordinatesFilter()
    : Superclass()
{
}

AssignPointAttributeToCoordinatesFilter::~AssignPointAttributeToCoordinatesFilter() = default;

int AssignPointAttributeToCoordinatesFilter::ExecuteInformation(vtkInformation * request, vtkInformationVector ** inputVector, vtkInformationVector * outputVector)
{
    if (!Superclass::ExecuteInformation(request, inputVector, outputVector))
    {
        return 0;
    }

    if (this->AttributeArrayToAssign.length() == 0)
    {
        vtkErrorMacro("No array name specified to assign to point coordinates.");
        return 0;
    }

    return 1;
}

int AssignPointAttributeToCoordinatesFilter::RequestData(
    vtkInformation * /*request*/,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inData = vtkPointSet::SafeDownCast(inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
    auto outData = vtkPointSet::SafeDownCast(outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));

    auto previousPointCoords = inData->GetPoints()->GetData();
    auto newPointsCoords = inData->GetPointData()->GetArray(this->AttributeArrayToAssign.c_str());
    if (!newPointsCoords)
    {
        vtkErrorMacro("Array to assign not found in input data: " + this->AttributeArrayToAssign);
        return 0;
    }

    if (newPointsCoords->GetNumberOfComponents() != 3
        || newPointsCoords->GetNumberOfTuples() != inData->GetNumberOfPoints())
    {
        vtkErrorMacro("Component/Tuple count mismatching in selected data array: " + this->AttributeArrayToAssign);
        return 0;
    }

    outData->ShallowCopy(inData);

    vtkNew<vtkPoints> newPoints;
    newPoints->SetData(newPointsCoords);
    outData->SetPoints(newPoints.Get());
    
    // pass current point coordinates as point attribute
    if (previousPointCoords)
    {
        outData->GetPointData()->AddArray(previousPointCoords);
    }

    return 1;
}
