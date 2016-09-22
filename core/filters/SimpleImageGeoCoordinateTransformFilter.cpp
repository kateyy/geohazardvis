#include <core/filters/SimpleImageGeoCoordinateTransformFilter.h>

#include <vtkImageChangeInformation.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <core/CoordinateSystems.h>
#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>


vtkStandardNewMacro(SimpleImageGeoCoordinateTransformFilter);


SimpleImageGeoCoordinateTransformFilter::SimpleImageGeoCoordinateTransformFilter()
    : Superclass()
    , SourceCoordinateSystem{ CoordinateSystem::UNSPECIFIED }
    , TargetCoordinateSystem{ CoordinateSystem::LOCAL_METRIC }
    , Step1{ vtkSmartPointer<vtkImageChangeInformation>::New() }
    , Step2{ vtkSmartPointer<vtkImageChangeInformation>::New() }
{
    this->Step2->SetInputConnection(
        this->Step1->GetOutputPort());
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
    this->Step2->UpdateDataObject();

    outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(),
        this->Step2->GetOutput());

    return 1;
}

int SimpleImageGeoCoordinateTransformFilter::RequestInformation(vtkInformation * request, 
    vtkInformationVector ** inputVector, vtkInformationVector * outputVector)
{
    this->SourceCoordinateSystem = UNSPECIFIED;

    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto inData = inInfo->Get(vtkDataObject::DATA_OBJECT());

    this->Step1->SetInputData(inData);

    const auto inCoordinateSytem = [inInfo, inData] ()
    {
        auto spec = ReferencedCoordinateSystemSpecification::fromInformation(*inData->GetInformation());
        if (spec.isValid(false))
        {
            return spec;
        }

        spec.fromInformation(*inInfo);
        return spec;
    }();

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

    DataBounds inBounds;

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
        vtkErrorMacro("Missing input data bounds information.");
        return 0;
    }

    auto outCoordinateSystem = inCoordinateSytem;
    outCoordinateSystem.type = (this->TargetCoordinateSystem == GLOBAL_GEOGRAPHIC)
        ? CoordinateSystemType::geographic
        : CoordinateSystemType::metricLocal;


    SetFilterParameters(outCoordinateSystem);

    this->Step2->UpdateInformation();
    auto internalOutInfo = this->Step2->GetOutputInformation(0);

    auto outInfo = outputVector->GetInformationObject(0);

    vtkVector3d newSpacing, newOrigin;
    ImageExtent newWholeExtent;

    if (this->SourceCoordinateSystem != this->TargetCoordinateSystem)
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

    outCoordinateSystem.writeToInformation(*outInfo);
    outCoordinateSystem.writeToInformation(*this->Step2->GetOutput()->GetInformation());

    return Superclass::RequestInformation(request, inputVector, outputVector);
}

int SimpleImageGeoCoordinateTransformFilter::RequestData(vtkInformation * /*request*/,
    vtkInformationVector ** /*inputVector*/, vtkInformationVector * /*outputVector*/)
{
    return this->Step2->GetExecutive()->Update();
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
            1.0
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
