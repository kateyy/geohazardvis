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

#include "ImageBlankNonFiniteValuesFilter.h"

#include <vtkAssume.h>
#include <vtkArrayDispatch.h>
#include <vtkDataArrayAccessor.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkSMPTools.h>
#include <vtkUniformGrid.h>
#include <vtkUnsignedCharArray.h>


vtkStandardNewMacro(ImageBlankNonFiniteValuesFilter);


namespace
{

struct ConvertAndMaskToUniformGridWorker
{
    vtkImageData & input;
    vtkUniformGrid & output;

    template<typename ArrayT>
    void operator()(ArrayT * scalars)
    {
        vtkDataArrayAccessor<ArrayT> s(scalars);

        const vtkIdType numTuples = scalars->GetNumberOfTuples();
        const int numComponents = scalars->GetNumberOfComponents();

        output.AllocatePointGhostArray();
        auto & ghostArray = *output.GetPointGhostArray();

        VTK_ASSUME(ghostArray.GetNumberOfValues() == numTuples);

        vtkSMPTools::For(0, numTuples,
            [numComponents, &s, &ghostArray] (vtkIdType begin, vtkIdType end)
        {
            for (auto tuple = begin; tuple < end; ++tuple)
            {
                for (int component = 0; component < numComponents; ++component)
                {
                    if (!std::isfinite(s.Get(tuple, component)))
                    {
                        // see vtkUniformGrid::BlankPoint
                        ghostArray.SetValue(tuple,
                            ghostArray.GetValue(tuple) | vtkDataSetAttributes::HIDDENPOINT);
                        break;
                    }
                }
            }
        });
    }
};

}


ImageBlankNonFiniteValuesFilter::ImageBlankNonFiniteValuesFilter()
    : Superclass()
{
}

ImageBlankNonFiniteValuesFilter::~ImageBlankNonFiniteValuesFilter() = default;

int ImageBlankNonFiniteValuesFilter::FillOutputPortInformation(int /*port*/, vtkInformation * info)
{
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUniformGrid");

    return 1;
}

int ImageBlankNonFiniteValuesFilter::RequestData(
    vtkInformation * /*request*/,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);

    auto inImage = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    auto outGrid = vtkUniformGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    outGrid->ShallowCopy(inImage);

    auto scalars = inImage->GetPointData()->GetScalars();

    if (!scalars)
    {
        // nothing to do
        return 1;
    }

    ConvertAndMaskToUniformGridWorker worker{ *inImage, *outGrid };

    // Dispatching float/double only, for all other scalar types this filter does nothing
    // (as other types don't have a NaN-equivalent).
    using RealsDispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
    RealsDispatcher::Execute(scalars, worker);

    return 1;
}
