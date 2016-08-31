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
    void setLightDirection(const vtkVector3d & lightDirection)
    {
        invNormLightDirection = -lightDirection.Normalized();
    }

    vtkVector3d invNormLightDirection;
    double ambientRatio = 0.0;

    template <typename NormalsArrayType, typename OutputArrayType>
    void operator() (NormalsArrayType * normals, OutputArrayType * lightness)
    {
        VTK_ASSUME(normals->GetNumberOfComponents() == 3);
        VTK_ASSUME(lightness->GetNumberOfComponents() == 1);

        using NormalsValueType = typename vtkDataArrayAccessor<NormalsArrayType>::APIType;
        using OutputValueType = typename vtkDataArrayAccessor<OutputArrayType>::APIType;

        const auto lightDir = convertTo<OutputValueType>(invNormLightDirection);
        const auto ambient = this->ambientRatio;

        vtkSMPTools::For(0, normals->GetNumberOfTuples(),
            [ambient, lightDir, normals, lightness] (vtkIdType begin, vtkIdType end)
        {
            vtkDataArrayAccessor<NormalsArrayType> n(normals);
            vtkDataArrayAccessor<OutputArrayType> l(lightness);

            vtkVector3<NormalsValueType> normal;

            for (auto tupleIdx = begin; tupleIdx < end; ++tupleIdx)
            {
                n.Get(tupleIdx, normal.GetData());

                const auto df = std::max(
                    static_cast<OutputValueType>(0.0),
                    static_cast<OutputValueType>(convertTo<OutputValueType>(normal).Dot(lightDir))
                );

                l.Set(tupleIdx, 0, static_cast<OutputValueType>(ambient + (1.0 - ambient) * df));
            }
        });
    }
};

}

DEMShadingFilter::DEMShadingFilter()
    : Superclass()
    , AmbientRatio{ 0.0 }
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

    if (auto inScalarInfo = vtkDataObject::GetActiveFieldInformation(inInfo,
        vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS))
    {
        numTuples = inScalarInfo->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES());
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

    DEMShadingWorker worker;
    worker.setLightDirection({ 1, 1, 0 });  // Light in the north west
    worker.ambientRatio = this->AmbientRatio;

    using Dispatcher = vtkArrayDispatch::Dispatch2ByValueType<
        vtkArrayDispatch::Reals,
        vtkArrayDispatch::Reals>;

    if (!Dispatcher::Execute(normals, lightness, worker))
    {
        worker(normals, lightness);
    }

    return 1;
}
