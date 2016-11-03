#include "AssignPointAttributeToCoordinatesFilter.h"

#include <cassert>

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPointSet.h>


vtkStandardNewMacro(AssignPointAttributeToCoordinatesFilter);


AssignPointAttributeToCoordinatesFilter::AssignPointAttributeToCoordinatesFilter()
    : Superclass()
    , CurrentCoordinatesAsScalars{ false }
{
}

AssignPointAttributeToCoordinatesFilter::~AssignPointAttributeToCoordinatesFilter() = default;

int AssignPointAttributeToCoordinatesFilter::RequestData(
    vtkInformation * /*request*/,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inData = vtkPointSet::SafeDownCast(inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
    auto outData = vtkPointSet::SafeDownCast(outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));

    auto previousPointCoords = inData->GetPoints()->GetData();
    vtkDataArray * pointsToAssign = nullptr;
    int pointsToAssignAttributeIndex = -1;

    if (!this->AttributeArrayToAssign.empty())
    {
        auto newPoints = inData->GetPointData()->GetArray(
            this->AttributeArrayToAssign.c_str(),
            pointsToAssignAttributeIndex);
        if (!newPoints)
        {
            vtkErrorMacro("Array to assign not found in input data: " + this->AttributeArrayToAssign);
        }

        if (newPoints->GetNumberOfComponents() != 3
            || newPoints->GetNumberOfTuples() != inData->GetNumberOfPoints())
        {
            vtkErrorMacro("Component/Tuple count mismatching in selected data array: " + this->AttributeArrayToAssign);
        }
        pointsToAssign = newPoints;
    }

    outData->ShallowCopy(inData);

    if (pointsToAssign)
    {
        vtkNew<vtkPoints> newPoints;
        newPoints->SetData(pointsToAssign);
        outData->SetPoints(newPoints.Get());
    }
    
    // pass current point coordinates as point attribute
    if (previousPointCoords)
    {
        outData->GetPointData()->AddArray(previousPointCoords);
    }

    if (auto currentCoords = pointsToAssign ? pointsToAssign : previousPointCoords)
    {
        outData->GetPointData()->SetActiveScalars(currentCoords->GetName());
    }

    return 1;
}
