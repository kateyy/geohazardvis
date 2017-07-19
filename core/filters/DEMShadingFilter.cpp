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

#include "DEMShadingFilter.h"

#include <algorithm>

#include <vtkArrayDispatch.h>
#include <vtkAssume.h>
#include <vtkCellData.h>
#include <vtkDataArrayAccessor.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkImageData.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkSMPTools.h>
#include <vtkVector.h>

#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>


vtkStandardNewMacro(DEMShadingFilter);

namespace
{

struct DEMShadingWorker
{
    void setDirectionToLight(const vtkVector3d & toLight)
    {
        this->toLightDirection = toLight.Normalized();
    }

    vtkVector3d toLightDirection;
    double diffuse = 1.0;
    double ambient = 0.0;

    template <typename NormalsArrayType, typename OutputArrayType>
    void operator() (NormalsArrayType * normals, OutputArrayType * lightness)
    {
        VTK_ASSUME(normals->GetNumberOfComponents() == 3);
        VTK_ASSUME(lightness->GetNumberOfComponents() == 1);

        using NormalsValueType = typename vtkDataArrayAccessor<NormalsArrayType>::APIType;
        using OutputValueType = typename vtkDataArrayAccessor<OutputArrayType>::APIType;

        const auto toLight = convertTo<OutputValueType>(toLightDirection);
        const auto d = this->diffuse;
        const auto a = this->ambient;

        vtkSMPTools::For(0, normals->GetNumberOfTuples(),
            [d, a, toLight, normals, lightness] (vtkIdType begin, vtkIdType end)
        {
            vtkDataArrayAccessor<NormalsArrayType> n(normals);
            vtkDataArrayAccessor<OutputArrayType> l(lightness);

            vtkVector3<NormalsValueType> normal;

            for (auto tupleIdx = begin; tupleIdx < end; ++tupleIdx)
            {
                n.Get(tupleIdx, normal.GetData());

                const auto df = std::max(
                    static_cast<OutputValueType>(0.0),
                    convertTo<OutputValueType>(normal).Dot(toLight)
                );

                l.Set(tupleIdx, 0, static_cast<OutputValueType>(std::min(1.0, a + d * df)));
            }
        });
    }
};

}

DEMShadingFilter::DEMShadingFilter()
    : Superclass()
    , Diffuse{ 1.0 }
    , Ambient{ 0.2 }
{
}

DEMShadingFilter::~DEMShadingFilter() = default;

int DEMShadingFilter::RequestInformation(vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    if (!Superclass::RequestInformation(request, inputVector, outputVector))
    {
        return 0;
    }


    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);

    const int scalarType = VTK_FLOAT;
    int numTuples = -1;
    const int numComponents = 1;
    const auto scalarsName = "Lightness";

    if (auto inNormalsInfo = vtkDataObject::GetActiveFieldInformation(inInfo,
        vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::NORMALS))
    {
        numTuples = inNormalsInfo->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES());
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

    vtkDataObject::SetActiveAttributeInfo(outInfo,
        vtkImageData::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS, scalarsName,
        scalarType, numComponents, numTuples);

    return 1;
}

int DEMShadingFilter::RequestData(vtkInformation * /*request*/,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);

    auto inImage = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    auto outImage = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    auto normals = inImage->GetPointData()->GetNormals();
    if (!normals)
    {
        vtkErrorMacro("Missing input normals");
        return 0;
    }

    outImage->CopyStructure(inImage);
    outImage->GetPointData()->PassData(inImage->GetPointData());
    outImage->GetCellData()->PassData(inImage->GetCellData());

    outImage->GetPointData()->SetActiveScalars(nullptr);

    outImage->AllocateScalars(outInfo);

    auto lightness = outImage->GetPointData()->GetScalars();
    lightness->SetName("Lightness");

    DEMShadingWorker worker;
    worker.setDirectionToLight({ -1, 1, 1 });  // Light from north west
    worker.diffuse = this->Diffuse;
    worker.ambient = this->Ambient;

    using Dispatcher = vtkArrayDispatch::Dispatch2ByValueType<
        vtkArrayDispatch::Reals,
        vtkArrayDispatch::Reals>;

    if (!Dispatcher::Execute(normals, lightness, worker))
    {
        worker(normals, lightness);
    }

    return 1;
}
