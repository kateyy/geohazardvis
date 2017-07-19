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

#include "SetMaskedPointScalarsToNaNFilter.h"

#include <vtkArrayDispatch.h>
#include <vtkAssume.h>
#include <vtkCellData.h>
#include <vtkCharArray.h>
#include <vtkDataArray.h>
#include <vtkDataArrayAccessor.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkProbeFilter.h>
#include <vtkSMPTools.h>


vtkStandardNewMacro(SetMaskedPointScalarsToNaNFilter);


namespace
{

struct SetMaskedPointScalarsToNaNWorker
{
    vtkCharArray & maskPoints;

    template<typename ArrayT>
    void operator()(ArrayT * scalars)
    {
        using ValueType = typename vtkDataArrayAccessor<ArrayT>::APIType;

        VTK_ASSUME(maskPoints.GetNumberOfComponents() == 1);

        const vtkIdType numTuples = scalars->GetNumberOfTuples();
        VTK_ASSUME(numTuples == maskPoints.GetNumberOfValues());

        vtkDataArrayAccessor<ArrayT> s(scalars);

        const int numComponents = scalars->GetNumberOfComponents();
        vtkSMPTools::For(0, numTuples,
            [numComponents, &s, this] (vtkIdType begin, vtkIdType end)
        {
            for (auto tuple = begin; tuple < end; ++tuple)
            {
                const bool isValid = maskPoints.GetValue(tuple) == static_cast<char>(1);
                if (isValid)
                {
                    continue;
                }

                for (int component = 0; component < numComponents; ++component)
                {
                    s.Set(tuple, component, std::numeric_limits<ValueType>::quiet_NaN());
                }
            }
        });
    }
};

}

SetMaskedPointScalarsToNaNFilter::SetMaskedPointScalarsToNaNFilter()
    : Superclass()
    , ValidPointMaskArrayName{ GetDefaultValidPointMaskArrayName() }
{
}

SetMaskedPointScalarsToNaNFilter::~SetMaskedPointScalarsToNaNFilter() = default;

const vtkStdString & SetMaskedPointScalarsToNaNFilter::GetDefaultValidPointMaskArrayName()
{
    static const auto defaultName = [] () -> vtkStdString
    {
        vtkNew<vtkProbeFilter> probe;
        return probe->GetValidPointMaskArrayName();
    }();
    return defaultName;
}

int SetMaskedPointScalarsToNaNFilter::RequestData(
    vtkInformation * /*request*/,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);

    auto inData = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    auto outData = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    outData->CopyStructure(inData);
    outData->GetPointData()->PassData(inData->GetPointData());
    outData->GetCellData()->PassData(inData->GetCellData());

    auto maskPoints = vtkCharArray::FastDownCast(inData->GetPointData()->GetAbstractArray(
        this->ValidPointMaskArrayName));
    if (!maskPoints)
    {
        vtkWarningMacro(<< R"(Point mask (char) array not found: ")"
            << this->ValidPointMaskArrayName << '"');
        return 1;
    }

    auto scalars = inData->GetPointData()->GetScalars();
    if (!scalars)
    {
        // nothing to do
        return 1;
    }

    SetMaskedPointScalarsToNaNWorker worker{ *maskPoints };

    // Dispatching float/double only, for all other scalar types this filter does nothing
    // (as other types don't have a NaN-equivalent).
    using RealsDispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
    RealsDispatcher::Execute(scalars, worker);

    return 1;
}
