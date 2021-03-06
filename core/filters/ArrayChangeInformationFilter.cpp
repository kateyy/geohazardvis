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

#include <core/utility/types_utils.h>
#include <core/utility/macros.h>


vtkStandardNewMacro(ArrayChangeInformationFilter);


namespace
{

vtkDataArray * extractAttribute(vtkDataSet & dataSet,
    IndexType location, int attributeType,
    vtkDataSetAttributes * & dsAttributes)
{
    dsAttributes = IndexType_util(location).extractAttributes(dataSet);

    if (!dsAttributes)
    {
        return nullptr;
    }

    return dsAttributes->GetAttribute(attributeType);
}

}


ArrayChangeInformationFilter::ArrayChangeInformationFilter()
    : AttributeLocation{ IndexType::points }
    , AttributeType{ vtkDataSetAttributes::SCALARS }
    , EnableRename{ true }
    , EnableSetUnit{ false }
    , PassInputArray{ false }
{
}

ArrayChangeInformationFilter::~ArrayChangeInformationFilter() = default;

int ArrayChangeInformationFilter::RequestInformation(
    vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    if (!Superclass::RequestInformation(request, inputVector, outputVector))
    {
        return 0;
    }

    const int fieldAssociation = IndexType_util(this->AttributeLocation).toVtkFieldAssociation();

    auto inInfo = inputVector[0]->GetInformationObject(0);

    int arrayType = -1;
    int numComponents = -1;
    int numTuples = -1;
    const char * inName = nullptr;

    if (auto inFieldInfo = vtkDataObject::GetActiveFieldInformation(
        inInfo,
        fieldAssociation,
        this->AttributeType))
    {
        arrayType = inFieldInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE());
        numComponents = inFieldInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
        numTuples = inFieldInfo->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES());
        inName = inFieldInfo->Get(vtkDataObject::FIELD_NAME());
    }

    if (arrayType <= 0)
    {
        arrayType = -1;
    }
    if (numComponents <= 0)
    {
        numComponents = -1;
    }
    if (numTuples <= 0)
    {
        numTuples = -1;
    }

    auto outInfo = outputVector->GetInformationObject(0);
    vtkDataObject::SetActiveAttributeInfo(
        outInfo,
        fieldAssociation,
        this->AttributeType,
        this->EnableRename ? this->ArrayName.c_str() : inName,
        arrayType,
        numComponents,
        numTuples);

    return 1;
}

int ArrayChangeInformationFilter::RequestData(
    vtkInformation * /*request*/,
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
    if (!outAttribute                   // originial attribute not copied
        || outAttribute == inAttribute) // passed attribute only -> copy is required
    {
        outAttribute.TakeReference(inAttribute->NewInstance());
        outAttribute->DeepCopy(inAttribute);

        if (this->EnableRename)
        {
            outAttribute->SetName(this->ArrayName);
        }

        // replace the array (and active attribute) by a copy
        if (!this->PassInputArray || !this->EnableRename)
        {
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

            outDsAttributes->RemoveArray(outAttrIndexPassed);
        }
        const int outAttrIndexCopied = outDsAttributes->AddArray(outAttribute);
        outDsAttributes->SetActiveAttribute(outAttrIndexCopied, this->AttributeType);
    }

    if (this->EnableSetUnit)
    {
        outAttribute->GetInformation()->Set(vtkDataArray::UNITS_LABEL(), this->ArrayUnit);
    }

    return 1;
}
