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

#include "DEMApplyShadingToColors.h"

#include <algorithm>
#include <string>
#include <vector>

#include <vtkArrayDispatch.h>
#include <vtkAssume.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataArrayAccessor.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkImageData.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkSMPTools.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <core/utility/DataExtent.h>


vtkStandardNewMacro(DEMApplyShadingToColors);


namespace
{

struct DEMShadingToColorsWorker
{
    template <typename LightnessArrayType, typename ColorArrayType, typename OutputArrayType>
    void operator() (LightnessArrayType * lightness, ColorArrayType * colors, OutputArrayType * output)
    {
        VTK_ASSUME(lightness->GetNumberOfComponents() == 1);
        VTK_ASSUME(colors->GetNumberOfComponents() == output->GetNumberOfComponents());
        VTK_ASSUME(lightness->GetNumberOfTuples() == colors->GetNumberOfTuples());
        VTK_ASSUME(lightness->GetNumberOfTuples() == output->GetNumberOfTuples());

        using OutputValueType = typename vtkDataArrayAccessor<OutputArrayType>::APIType;

        vtkSMPTools::For(0, lightness->GetNumberOfTuples(),
            [lightness, colors, output] (vtkIdType begin, vtkIdType end)
        {
            vtkDataArrayAccessor<LightnessArrayType> l(lightness);
            vtkDataArrayAccessor<ColorArrayType> c(colors);
            vtkDataArrayAccessor<OutputArrayType> o(output);

            const int numComponents = std::min(3, colors->GetNumberOfComponents());
            const bool passAlpha = colors->GetNumberOfComponents() == 4;

            for (auto tupleIdx = begin; tupleIdx < end; ++tupleIdx)
            {
                const auto lightValue = static_cast<float>(l.Get(tupleIdx, 0));

                for (int component = 0; component < numComponents; ++component)
                {
                    const auto colorComp = static_cast<float>(c.Get(tupleIdx, component));
                    o.Set(tupleIdx, component,
                        static_cast<OutputValueType>(lightValue * colorComp));
                }
                if (passAlpha)
                {
                    o.Set(tupleIdx, 3,
                        static_cast<OutputValueType>(c.Get(tupleIdx, 3)));
                }
            }
        });
    }
};

}

DEMApplyShadingToColors::DEMApplyShadingToColors()
    : Superclass()
{
}

DEMApplyShadingToColors::~DEMApplyShadingToColors() = default;

int DEMApplyShadingToColors::RequestInformation(vtkInformation * request,
    vtkInformationVector ** inputVector, vtkInformationVector * outputVector)
{
    if (!Superclass::RequestInformation(request, inputVector, outputVector))
    {
        return 0;
    }

    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);

    const int scalarType = VTK_FLOAT;
    int numTuples = -1;
    int numComponents = 1;

    if (auto inScalarsInfo = vtkDataObject::GetActiveFieldInformation(inInfo,
        vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS))
    {
        numTuples = inScalarsInfo->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES());
        numComponents = inScalarsInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
    }
    if (numTuples <= 0)
    {
        ImageExtent extent;

        if (inInfo->Has(vtkDataObject::DATA_EXTENT()))
        {
            inInfo->Get(vtkDataObject::DATA_EXTENT(), extent.data());
        }
        else if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
        {
            inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent.data());
        }

        if (!extent.isEmpty())
        {
            numTuples = static_cast<decltype(numTuples)>(extent.numberOfPoints());
        }
    }

    if (numTuples <= 0)
    {
        numTuples = -1;
    }
    if (numComponents <= 0)
    {
        numComponents = -1;
    }

    vtkDataObject::SetActiveAttributeInfo(outInfo,
        vtkImageData::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS,
        "Colors/Shading",
        scalarType, numComponents, numTuples);

    return 1;
}

int DEMApplyShadingToColors::RequestData(vtkInformation * /*request*/,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);

    auto inImage = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    auto outImage = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    auto colors = inImage->GetPointData()->GetScalars();

    static const auto validComponents = std::vector<int>({ 1, 2, 3, 4 });

    if (!colors ||
        std::find(validComponents.begin(), validComponents.end(), colors->GetNumberOfComponents())
        == validComponents.end())
    {
        vtkErrorMacro("Current scalars do not contain valid color data");
        return 0;
    }

    auto lightness = inImage->GetPointData()->GetArray("Lightness");
    if (!lightness)
    {
        vtkErrorMacro("Missing lightness array");
        return 0;
    }

    outImage->CopyStructure(inImage);
    outImage->GetPointData()->PassData(inImage->GetPointData());
    outImage->GetCellData()->PassData(inImage->GetCellData());

    outImage->GetPointData()->SetActiveScalars(nullptr);

    outImage->AllocateScalars(colors->GetDataType(), colors->GetNumberOfComponents());
    auto output = outImage->GetPointData()->GetScalars();
    output->SetName("Colors/Shading");

    DEMShadingToColorsWorker worker;

    using Dispatcher = vtkArrayDispatch::Dispatch3ByValueType<
        vtkArrayDispatch::Reals,
        vtkArrayDispatch::AllTypes,
        vtkArrayDispatch::AllTypes>;

    if (!Dispatcher::Execute(lightness, colors, output, worker))
    {
        vtkWarningMacro("Data array dispatch failed");
        worker(lightness, colors, output);
    }

    return 1;
}
