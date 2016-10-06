#include <core/filters/SimplePolyGeoCoordinateTransformFilter.h>

#include <cassert>

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <core/CoordinateSystems.h>
#include <core/filters/SetCoordinateSystemInformationFilter.h>
#include <core/utility/vtkvectorhelper.h>


vtkStandardNewMacro(SimplePolyGeoCoordinateTransformFilter);


SimplePolyGeoCoordinateTransformFilter::SimplePolyGeoCoordinateTransformFilter()
    : Superclass()
    , SourceCoordinateSystem{ CoordinateSystem::UNSPECIFIED }
    , TargetCoordinateSystem{ CoordinateSystem::LOCAL_METRIC }
    , TransformFilter{ vtkSmartPointer<vtkTransformPolyDataFilter>::New() }
    , Transform{ vtkSmartPointer<vtkTransform>::New() }
    , InfoSetter{ vtkSmartPointer<SetCoordinateSystemInformationFilter>::New() }
{
    this->Transform->PostMultiply();
    this->TransformFilter->SetTransform(Transform);
    this->InfoSetter->SetInputConnection(this->TransformFilter->GetOutputPort());
}

SimplePolyGeoCoordinateTransformFilter::~SimplePolyGeoCoordinateTransformFilter() = default;

int SimplePolyGeoCoordinateTransformFilter::ProcessRequest(vtkInformation * request, vtkInformationVector ** inputVector, vtkInformationVector * outputVector)
{
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
        return this->RequestDataObject(request, inputVector, outputVector);
    }

    return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int SimplePolyGeoCoordinateTransformFilter::RequestDataObject(vtkInformation * /*request*/, vtkInformationVector ** /*inputVector*/, vtkInformationVector * outputVector)
{
    this->InfoSetter->UpdateDataObject();

    outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(),
        this->InfoSetter->GetOutput());

    return 1;
}

int SimplePolyGeoCoordinateTransformFilter::RequestInformation(vtkInformation * request, 
    vtkInformationVector ** inputVector, vtkInformationVector * outputVector)
{
    this->SourceCoordinateSystem = UNSPECIFIED;

    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto inData = inInfo->Get(vtkDataObject::DATA_OBJECT());

    this->TransformFilter->SetInputData(inData);

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

    if (!this->InfoSetter->GetExecutive()->UpdateInformation())
    {
        vtkErrorMacro("Error occurred while updating transform information.");
        return 0;
    }

    this->SetFilterParameters(outCoordinateSystem);

    return Superclass::RequestInformation(request, inputVector, outputVector);
}

int SimplePolyGeoCoordinateTransformFilter::RequestData(vtkInformation * /*request*/,
    vtkInformationVector ** /*inputVector*/, vtkInformationVector * /*outputVector*/)
{
    return this->InfoSetter->GetExecutive()->Update();
}

void SimplePolyGeoCoordinateTransformFilter::SetFilterParameters(const ReferencedCoordinateSystemSpecification & targetSpec)
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
            1.0     // do not change elevations
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

    this->Transform->Identity();
    this->Transform->Translate(preTranslate.GetData());
    this->Transform->Scale(scale.GetData());
    this->Transform->Translate(postTranslate.GetData());
}
