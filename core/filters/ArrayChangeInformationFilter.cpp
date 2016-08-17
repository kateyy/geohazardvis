#include "ArrayChangeInformationFilter.h"

#include <cassert>

#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkVersionMacros.h>

#include <core/utility/macros.h>


vtkStandardNewMacro(ArrayChangeInformationFilter);


namespace
{
vtkDataArray * extractAttribute(vtkDataSet & dataSet, 
    ArrayChangeInformationFilter::AttributeLocations location, int attributeType,
    vtkDataSetAttributes * & dsAttributes)
{
    dsAttributes = nullptr;

    switch (location)
    {
    case ArrayChangeInformationFilter::AttributeLocations::POINT_DATA:
        dsAttributes = dataSet.GetPointData();
        break;
    case ArrayChangeInformationFilter::AttributeLocations::CELL_DATA:
        dsAttributes = dataSet.GetCellData();
        break;
    default:
        return nullptr;
    }

    if (!dsAttributes)
    {
        return nullptr;
    }

    return dsAttributes->GetAttribute(attributeType);
}
}


ArrayChangeInformationFilter::ArrayChangeInformationFilter()
    : AttributeLocation{ AttributeLocations::POINT_DATA }
    , AttributeType{ vtkDataSetAttributes::SCALARS }
    , EnableRename{ true }
    , ArrayName{ nullptr }
    , EnableSetUnit{ false }
    , ArrayUnit{ nullptr }
{
}

ArrayChangeInformationFilter::~ArrayChangeInformationFilter()
{
    this->SetArrayName(nullptr);
    this->SetArrayUnit(nullptr);
}

int ArrayChangeInformationFilter::RequestData(
    vtkInformation *,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);

    auto dsInput = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    auto dsOutput = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    dsOutput->CopyStructure(dsInput);

    dsOutput->GetPointData()->PassData(dsInput->GetPointData());
    dsOutput->GetCellData()->PassData(dsInput->GetCellData());

    if (!this->EnableRename && !this->EnableSetUnit)
    {
        return 1;
    }

    vtkDataSetAttributes * inDsAttributes = nullptr, *outDsAttributes = nullptr;

    auto inAttribute = extractAttribute(*dsInput, this->AttributeLocation, this->AttributeType, inDsAttributes);

    if (!inAttribute)
    {
        return 0;
    }

    // Don't change array information in the upstream algorithm.
    // Instead, create a copy of the array and change information on this object.

    // Check if there already is a copy of the array
    vtkSmartPointer<vtkDataArray> outAttribute = extractAttribute(*dsOutput, 
        this->AttributeLocation, this->AttributeType, outDsAttributes);
    if (outAttribute == inAttribute) // passed attribute only -> copy is required
    {
        assert(outAttribute);

        outAttribute.TakeReference(inAttribute->NewInstance());
        outAttribute->DeepCopy(inAttribute);

        int outAttrIndexPassed = -1;
        for (int i = 0; i < inDsAttributes->GetNumberOfArrays(); ++i)
        {
            if (inDsAttributes->GetArray(i) == inAttribute)
            {
                outAttrIndexPassed = i;
                break;
            }
        }

        assert(outAttrIndexPassed != -1);

        // replace the array (and active attribute) by a copy
        outDsAttributes->RemoveArray(outAttrIndexPassed);
        const int outAttrIndexCopied = outDsAttributes->AddArray(outAttribute);
        outDsAttributes->SetActiveAttribute(outAttrIndexCopied, this->AttributeType);
    }

    if (this->EnableRename)
    {
        outAttribute->SetName(this->ArrayName);
    }

#if VTK_CHECK_VERSION(7, 1, 0)
    if (this->EnableSetUnit)
    {
        outAttribute->GetInformation()->Set(vtkDataArray::UNITS_LABEL(), this->ArrayUnit);
    }
#endif

    return 1;
}