#include "DEMImageNormals.h"

#include <vtkArrayDispatch.h>
#include <vtkAssume.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataArrayAccessor.h>
#include <vtkExecutive.h>
#include <vtkImageConvolve.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkSMPTools.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkVector.h>

#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>


vtkStandardNewMacro(DEMImageNormals)


namespace
{

struct CalcNormalsWorker
{
    double ElevationUnitScale = 1.0;
    double CoordinatesUnitScale = 1.0;

    vtkVector2d xySpacing = { 1.0, 1.0 };

    template <typename ArrayType>
    void operator() (ArrayType * genericI_x, ArrayType * genericI_y, ArrayType * normals)
    {
        VTK_ASSUME(genericI_x->GetNumberOfComponents() == 1);
        VTK_ASSUME(genericI_y->GetNumberOfComponents() == 1);
        VTK_ASSUME(normals->GetNumberOfComponents() == 3);

        const vtkIdType numTuples = normals->GetNumberOfTuples();

        using ValueType = typename vtkDataArrayAccessor<ArrayType>::APIType;

        const auto spacing = this->xySpacing * this->CoordinatesUnitScale;
        const auto scale = convertTo<ValueType>(this->ElevationUnitScale * spacing);

        vtkSMPTools::For(0, normals->GetNumberOfTuples(),
            [genericI_x, genericI_y, normals, scale]
        (vtkIdType begin, vtkIdType end)
        {
            vtkDataArrayAccessor<ArrayType> i_x(genericI_x);
            vtkDataArrayAccessor<ArrayType> i_y(genericI_y);
            vtkDataArrayAccessor<ArrayType> n(normals);

            vtkVector3<ValueType> normal;

            for (auto tupleIdx = begin; tupleIdx < end; ++tupleIdx)
            {
                normal = {
                    -i_x.Get(tupleIdx, 0) * scale[0],
                    -i_y.Get(tupleIdx, 0) * scale[1],
                    1.0
                };
                normal.Normalize();

                n.Set(tupleIdx, 0, normal.GetX());
                n.Set(tupleIdx, 1, normal.GetY());
                n.Set(tupleIdx, 2, normal.GetZ());
            }
        });
    }
};

}


DEMImageNormals::DEMImageNormals()
    : Superclass()
    , CoordinatesUnitScale{ 1000.0 } // coordinate in km by default
    , ElevationUnitScale{ 1.0 / 100.0 } // Elevations in cm by default
{
}

DEMImageNormals::~DEMImageNormals() = default;

int DEMImageNormals::RequestInformation(vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    if (!Superclass::RequestInformation(request, inputVector, outputVector))
    {
        return 0;
    }

    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);

    int scalarType = -1;
    int numTuples = -1;
    const int numComponents = 3;  // output: normals

    if (auto inScalarInfo = vtkDataObject::GetActiveFieldInformation(inInfo,
        vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS))
    {
        scalarType = inScalarInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE());
        numTuples = inScalarInfo->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES());
    }
    if (scalarType < 0)
    {
        scalarType = vtkImageData::GetScalarType(inInfo);
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
        vtkImageData::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::NORMALS, "Normals",
        scalarType, numComponents, numTuples);

    return 1;
}

int DEMImageNormals::RequestData(vtkInformation * /*request*/,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);

    auto inImage = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    auto outImage = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    auto elevations = inImage->GetPointData()->GetScalars();
    if (!elevations || elevations->GetNumberOfComponents() != 1 || !elevations->GetName())
    {
        vtkErrorMacro("Could not find valid elevations in image scalars.");
        return 0;
    }

    static const double kernel_x[3 * 3]{
        -1, 0, 1,
        -1, 0, 1,
        -1, 0, 1
    };
    static const double kernel_y[3 * 3]{
        1, 1, 1,
        0, 0, 0,
        -1, -1, -1
    };

    auto computeIx = vtkSmartPointer<vtkImageConvolve>::New();
    computeIx->SetInputData(inImage);
    computeIx->SetKernel3x3(kernel_x);

    auto computeIy = vtkSmartPointer<vtkImageConvolve>::New();
    computeIy->SetInputData(inImage);
    computeIy->SetKernel3x3(kernel_y);

    vtkSmartPointer<vtkDataArray> i_x, i_y;

    if (computeIx->GetExecutive()->Update())
    {
        i_x = computeIx->GetOutput()->GetPointData()->GetScalars();
    }
    if (i_x && computeIy->GetExecutive()->Update())
    {
        i_y = computeIy->GetOutput()->GetPointData()->GetScalars();
    }

    const auto numberOfPoints = inImage->GetNumberOfPoints();

    if (!i_x || !i_y
        || (i_x->GetNumberOfTuples() != i_y->GetNumberOfTuples())
        || (i_x->GetNumberOfTuples() != numberOfPoints))
    {
        vtkErrorMacro("Could not convolve input image for normals computation");
        return 0;
    }

    outImage->CopyStructure(inImage);
    outImage->GetPointData()->PassData(inImage->GetPointData());
    outImage->GetCellData()->PassData(inImage->GetCellData());

    vtkSmartPointer<vtkDataArray> normals = outImage->GetPointData()->GetArray("Normals");
    if (!normals
        || normals->GetDataType() != i_x->GetDataType()
        || normals->GetNumberOfTuples() != numberOfPoints
        || normals->GetNumberOfComponents() != 3)
    {
        normals.TakeReference(elevations->NewInstance());
        normals->SetNumberOfComponents(3);
        normals->SetNumberOfTuples(numberOfPoints);
    }

    outImage->GetPointData()->SetNormals(normals);

    using Dispatcher = vtkArrayDispatch::Dispatch3BySameValueType<vtkArrayDispatch::Reals>;

    vtkVector3d spacing;
    outImage->GetSpacing(spacing.GetData());

    CalcNormalsWorker worker;
    worker.ElevationUnitScale = this->ElevationUnitScale;
    worker.CoordinatesUnitScale = this->CoordinatesUnitScale;
    worker.xySpacing = { spacing[0], spacing[1] };

    if (!Dispatcher::Execute(i_x, i_y, normals, worker))
    {
        worker(i_x.Get(), i_y.Get(), normals.Get());
    }

    return 1;
}
