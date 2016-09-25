#include <core/filters/SimpleImageGeoCoordinateTransformFilter.h>

#include <vtkDataObject.h>
#include <vtkImageChangeInformation.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkInformationIntegerVectorKey.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <core/CoordinateSystems.h>
#include <core/filters/SetCoordinateSystemInformationFilter.h>
#include <core/utility/vtkvectorhelper.h>


vtkStandardNewMacro(SimpleImageGeoCoordinateTransformFilter);


SimpleImageGeoCoordinateTransformFilter::SimpleImageGeoCoordinateTransformFilter()
    : Superclass()
    , SourceCoordinateSystem{ CoordinateSystem::UNSPECIFIED }
    , TargetCoordinateSystem{ CoordinateSystem::LOCAL_METRIC }
    , Step1{ vtkSmartPointer<vtkImageChangeInformation>::New() }
    , Step2{ vtkSmartPointer<vtkImageChangeInformation>::New() }
    , InfoSetter{ vtkSmartPointer<SetCoordinateSystemInformationFilter>::New() }
{
    this->Step2->SetInputConnection(this->Step1->GetOutputPort());
    this->InfoSetter->SetInputConnection(this->Step2->GetOutputPort());
}

SimpleImageGeoCoordinateTransformFilter::~SimpleImageGeoCoordinateTransformFilter() = default;

int SimpleImageGeoCoordinateTransformFilter::ProcessRequest(vtkInformation * request, vtkInformationVector ** inputVector, vtkInformationVector * outputVector)
{
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
        return this->RequestDataObject(request, inputVector, outputVector);
    }

    return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int SimpleImageGeoCoordinateTransformFilter::RequestDataObject(vtkInformation * /*request*/, vtkInformationVector ** /*inputVector*/, vtkInformationVector * outputVector)
{
    this->InfoSetter->UpdateDataObject();

    outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(),
        this->InfoSetter->GetOutputDataObject(0));

    return 1;
}

int SimpleImageGeoCoordinateTransformFilter::RequestInformation(vtkInformation * request, 
    vtkInformationVector ** inputVector, vtkInformationVector * outputVector)
{
    this->SourceCoordinateSystem = UNSPECIFIED;

    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto inData = inInfo->Get(vtkDataObject::DATA_OBJECT());

    this->Step1->SetInputData(inData);

    auto inCoordinateSytem = ReferencedCoordinateSystemSpecification::fromInformation(*inInfo);
    if (!inCoordinateSytem.isValid(false)
        || !inCoordinateSytem.isReferencePointValid())
    {
        inCoordinateSytem = ReferencedCoordinateSystemSpecification::fromFieldData(*inData->GetFieldData());
    }

    if (!inCoordinateSytem.isValid(false)
        || !inCoordinateSytem.isReferencePointValid())
    {
        vtkErrorMacro("Missing input coordinate system information.");
        return 0;
    }

    if (inCoordinateSytem.type == CoordinateSystemType::geographic)
    {
        this->SourceCoordinateSystem = GLOBAL_GEOGRAPHIC;
    }
    else if (inCoordinateSytem.type == CoordinateSystemType::metricLocal)
    {
        this->SourceCoordinateSystem = LOCAL_METRIC;
    }
    else
    {
        vtkErrorMacro("Unsupported input coordinate system information.");
        return 0;
    }

    auto outCoordinateSystem = inCoordinateSytem;
    outCoordinateSystem.type = (this->TargetCoordinateSystem == GLOBAL_GEOGRAPHIC)
        ? CoordinateSystemType::geographic
        : CoordinateSystemType::metricLocal;

    this->InfoSetter->SetCoordinateSystemSpec(outCoordinateSystem);


    SetFilterParameters(outCoordinateSystem);

    if (!this->InfoSetter->GetExecutive()->UpdateInformation())
    {
        vtkErrorMacro("Error occurred while updating transform information.");
        return 0;
    }

    auto internalOutInfo = this->InfoSetter->GetOutputInformation(0);
    auto outInfo = outputVector->GetInformationObject(0);

    // Pass through available upstream information...
    outInfo->Copy(internalOutInfo);

    // and overwrite changed values.

    vtkVector3d newSpacing, newOrigin;

    if (this->SourceCoordinateSystem != this->TargetCoordinateSystem)
    {
        internalOutInfo->Get(vtkDataObject::SPACING(), newSpacing.GetData());
        internalOutInfo->Get(vtkDataObject::ORIGIN(), newOrigin.GetData());
    }
    else
    {
        inInfo->Get(vtkDataObject::SPACING(), newSpacing.GetData());
        inInfo->Get(vtkDataObject::ORIGIN(), newOrigin.GetData());
    }

    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
        outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
            inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
            vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()->Length(inInfo));
    }
    outInfo->Set(vtkDataObject::SPACING(), newSpacing.GetData(), newSpacing.GetSize());
    outInfo->Set(vtkDataObject::ORIGIN(), newOrigin.GetData(), newOrigin.GetSize());

    return Superclass::RequestInformation(request, inputVector, outputVector);
}

int SimpleImageGeoCoordinateTransformFilter::RequestData(vtkInformation * /*request*/,
    vtkInformationVector ** /*inputVector*/, vtkInformationVector * /*outputVector*/)
{
    return this->InfoSetter->GetExecutive()->Update();
}

void SimpleImageGeoCoordinateTransformFilter::SetFilterParameters(const ReferencedCoordinateSystemSpecification & targetSpec)
{
    // approximations for regions not larger than a few hundreds of kilometers:
     /*
     auto transformApprox = [earthR] (double Phi, double La, double Phi0, double La0, double & X, double & Y)
     {
     X = earthR * (La - La0) * std::cos(Phi0 * vtkMath::Pi() / 180) * vtkMath::Pi() / 180;
     Y = earthR * (Phi - Phi0) * vtkMath::Pi() / 180;
     };

     reverse:
     La = X / (earthR * std::cos(Phi0 * vtkMath::Pi() / 180)) / vtkMath::Pi() * 180 + La0;
     Phi = Y / (earthR * vtkMath::Pi()) * 180 + Phi0;
     */

    static const double earthR = 6378.138;

    const auto La0 = targetSpec.referencePointLatLong[1];   // average longitude (Greek lambda)
    const auto Phi0 = targetSpec.referencePointLatLong[0];   // average latitude (Greek phi)

    const auto cosPhi0 = std::cos(vtkMath::RadiansFromDegrees(Phi0));
    static const auto toRadians = vtkMath::Pi() / 180.0;
    static const auto toDegrees = 180.0 / vtkMath::Pi();

    vtkVector3d preTranslate = { 0, 0, 0 };
    vtkVector3d scale = { 1, 1, 1 };
    vtkVector3d postTranslate = { 0, 0, 0 };

    if (this->SourceCoordinateSystem == GLOBAL_GEOGRAPHIC
        && this->TargetCoordinateSystem == LOCAL_METRIC)
    {

        preTranslate = vtkVector3d{
            -La0,
            -Phi0,
            0.0
        };
        scale = vtkVector3d{
            toRadians * earthR * cosPhi0,
            toRadians * earthR,
            0.0  // flattening, elevation is stored in scalars
        };
    }
    else if (this->SourceCoordinateSystem == LOCAL_METRIC
            && this->TargetCoordinateSystem == GLOBAL_GEOGRAPHIC)
    {
        scale = vtkVector3d{
            toDegrees / (earthR * cosPhi0),
            toDegrees / earthR,
            0.0
        };
        postTranslate = vtkVector3d{
            La0,
            Phi0,
            0.0
        };
    }

    this->Step1->SetOriginTranslation(preTranslate.GetData());
    this->Step2->SetOriginScale(scale.GetData());
    this->Step2->SetSpacingScale(scale.GetData());
    this->Step2->SetOriginTranslation(postTranslate.GetData());
}
