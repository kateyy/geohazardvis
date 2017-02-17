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
