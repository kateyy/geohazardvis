#include <core/filters/SimpleDEMGeoCoordToLocalFilter.h>

#include <vtkImageChangeInformation.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>


vtkStandardNewMacro(SimpleDEMGeoCoordToLocalFilter);


SimpleDEMGeoCoordToLocalFilter::SimpleDEMGeoCoordToLocalFilter()
    : Superclass()
    , Enabled{ true }
    , UseNorthWestAsOrigin{ true }
    , GeoOrigin{ 0.0, 0.0 }
    , TranslateFilter{ vtkSmartPointer<vtkImageChangeInformation>::New() }
    , ScaleFilter{ vtkSmartPointer<vtkImageChangeInformation>::New() }
{
    this->ScaleFilter->SetInputConnection(
        this->TranslateFilter->GetOutputPort());
}

SimpleDEMGeoCoordToLocalFilter::~SimpleDEMGeoCoordToLocalFilter() = default;

int SimpleDEMGeoCoordToLocalFilter::ProcessRequest(vtkInformation * request, vtkInformationVector ** inputVector, vtkInformationVector * outputVector)
{
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
        return this->RequestDataObject(request, inputVector, outputVector);
    }

    return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int SimpleDEMGeoCoordToLocalFilter::RequestDataObject(vtkInformation * /*request*/, vtkInformationVector ** /*inputVector*/, vtkInformationVector * outputVector)
{
    this->ScaleFilter->UpdateDataObject();

    outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(),
        this->ScaleFilter->GetOutput());

    return 1;
}

const vtkVector2d & SimpleDEMGeoCoordToLocalFilter::GetGeoOrigin()
{
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning GeoOrigin of " << this->GeoOrigin);
    return this->GeoOrigin;
}

int SimpleDEMGeoCoordToLocalFilter::RequestInformation(vtkInformation * request, 
    vtkInformationVector ** inputVector, vtkInformationVector * outputVector)
{
    DataBounds inBounds;

    auto inInfo = inputVector[0]->GetInformationObject(0);

    this->TranslateFilter->SetInputData(inInfo->Get(vtkDataObject::DATA_OBJECT()));

    if (inInfo->Has(vtkDataObject::BOUNDING_BOX()))
    {
        inInfo->Get(vtkDataObject::BOUNDING_BOX(), inBounds.data());
    }
    else if (inInfo->Has(vtkStreamingDemandDrivenPipeline::BOUNDS()))
    {
        inInfo->Get(vtkStreamingDemandDrivenPipeline::BOUNDS(), inBounds.data());
    }
    else if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT())
        && inInfo->Has(vtkDataObject::SPACING()) && inInfo->Has(vtkDataObject::ORIGIN()))
    {
        auto boundsCheck = vtkSmartPointer<vtkImageData>::New();
        boundsCheck->SetExtent(inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
        boundsCheck->SetSpacing(inInfo->Get(vtkDataObject::SPACING()));
        boundsCheck->SetOrigin(inInfo->Get(vtkDataObject::ORIGIN()));

        boundsCheck->GetBounds(inBounds.data());
    }
    else
    {
        return 0;
    }

    SetParameters(inBounds.min());

    this->ScaleFilter->UpdateInformation();
    auto internalOutInfo = this->ScaleFilter->GetOutputInformation(0);

    auto outInfo = outputVector->GetInformationObject(0);

    vtkVector3d newSpacing, newOrigin;
    ImageExtent newWholeExtent;

    if (this->Enabled)
    {
        internalOutInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), newWholeExtent.data());
        internalOutInfo->Get(vtkDataObject::SPACING(), newSpacing.GetData());
        internalOutInfo->Get(vtkDataObject::ORIGIN(), newOrigin.GetData());
    }
    else
    {
        inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), newWholeExtent.data());
        inInfo->Get(vtkDataObject::SPACING(), newSpacing.GetData());
        inInfo->Get(vtkDataObject::ORIGIN(), newOrigin.GetData());
    }

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), newWholeExtent.data(), newWholeExtent.ValueCount);
    outInfo->Set(vtkDataObject::SPACING(), newSpacing.GetData(), newSpacing.GetSize());
    outInfo->Set(vtkDataObject::ORIGIN(), newOrigin.GetData(), newOrigin.GetSize());

    return Superclass::RequestInformation(request, inputVector, outputVector);
}

int SimpleDEMGeoCoordToLocalFilter::RequestData(vtkInformation * /*request*/,
    vtkInformationVector ** /*inputVector*/, vtkInformationVector * /*outputVector*/)
{
    return this->ScaleFilter->GetExecutive()->Update();
}

void SimpleDEMGeoCoordToLocalFilter::SetParameters(const vtkVector3d & inputGeoNorthWest)
{
    // approximations for regions not larger than a few hundreds of kilometers:
     /*
     auto transformApprox = [earthR] (double Fi, double La, double Fi0, double La0, double & X, double & Y)
     {
     Y = earthR * (Fi - Fi0) * vtkMath::Pi() / 180;
     X = earthR * (La - La0) * std::cos(Fi0 / 180 * vtkMath::Pi()) * vtkMath::Pi() / 180;
     };
     */

    vtkVector3d toLocalTranslation, toLocalScale;

    if (this->Enabled)
    {
        static const double earthR = 6378.138;

        const auto geoOrigin = this->UseNorthWestAsOrigin
            ? convertTo<2>(inputGeoNorthWest)
            : this->GeoOrigin;

        const auto Fi0 = geoOrigin[0];   // most western longitude (Greek lambda)
        const auto La0 = geoOrigin[1];   // most northern latitude (Greek phi)

        toLocalTranslation = vtkVector3d{
            -La0,
            -Fi0,
            0.0
        };
        toLocalScale = vtkVector3d{
            earthR * std::cos(Fi0) * vtkMath::Pi() / 180.0,
            earthR * vtkMath::Pi() / 180.0,
            0.0  // flattening, elevation is stored in scalars
        };
    }
    else
    {
        toLocalTranslation = { 0.0, 0.0, 0.0 };
        toLocalScale = { 1.0, 1.0, 1.0 };
    }

    this->TranslateFilter->SetOriginTranslation(toLocalTranslation.GetData());
    this->ScaleFilter->SetOriginScale(toLocalScale.GetData());
    this->ScaleFilter->SetSpacingScale(toLocalScale.GetData());
}
