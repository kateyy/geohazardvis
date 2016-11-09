#include <core/filters/AbstractSimpleGeoCoordinateTransformFilter.h>

#include <vtkDataObject.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkImageAlgorithm.h>
#include <vtkMath.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <core/utility/mathhelper.h>
#include <core/utility/vtkvectorhelper.h>


template<typename Superclass_t>
AbstractSimpleGeoCoordinateTransformFilter<Superclass_t>::AbstractSimpleGeoCoordinateTransformFilter()
    : Superclass()
    , SourceCoordinateSystem{}
    , TargetCoordinateSystem{}
    , TargetCoordinateSystemType{ CoordinateSystemType::metricLocal }
    , TargetMetricUnit{ "km" }
{
    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(1);
}

template<typename Superclass_t>
AbstractSimpleGeoCoordinateTransformFilter<Superclass_t>::~AbstractSimpleGeoCoordinateTransformFilter() = default;

template<typename Superclass_t>
void AbstractSimpleGeoCoordinateTransformFilter<Superclass_t>::SetTargetCoordinateSystemType(CoordinateSystemType type)
{
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting TargetCoordinateSystemType to " << type.value);
    if (this->TargetCoordinateSystemType != type)
    {
        this->TargetCoordinateSystemType = type;
        this->Modified();
    }
}

template<typename Superclass_t>
CoordinateSystemType AbstractSimpleGeoCoordinateTransformFilter<Superclass_t>::GetTargetCoordinateSystemType() const
{
    return this->TargetCoordinateSystemType;
}

template<typename Superclass_t>
int AbstractSimpleGeoCoordinateTransformFilter<Superclass_t>::ProcessRequest(
    vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
        return this->RequestDataObject(request, inputVector, outputVector);
    }
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
        return this->RequestInformation(request, inputVector, outputVector);
    }
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
        return this->RequestData(request, inputVector, outputVector);
    }

    return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

template<typename Superclass_t>
int AbstractSimpleGeoCoordinateTransformFilter<Superclass_t>::RequestDataObject(
    vtkInformation * /*request*/,
    vtkInformationVector ** /*inputVector*/,
    vtkInformationVector * /*outputVector*/)
{
    return 1;
}

template<typename Superclass_t>
int AbstractSimpleGeoCoordinateTransformFilter<Superclass_t>::RequestInformation(
    vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto inData = inInfo->Get(vtkDataObject::DATA_OBJECT());

    this->SourceCoordinateSystem = ReferencedCoordinateSystemSpecification::fromInformation(*inInfo);
    if (!this->SourceCoordinateSystem.isValid())
    {
        this->SourceCoordinateSystem = ReferencedCoordinateSystemSpecification::fromFieldData(*inData->GetFieldData());
    }
    if (!this->SourceCoordinateSystem.isValid())
    {
        vtkErrorMacro("Missing input coordinate system information.");
        return 0;
    }
    if (!this->SourceCoordinateSystem.isReferencePointValid()
        && this->SourceCoordinateSystem.type != this->TargetCoordinateSystemType)
    {
        vtkErrorMacro("Missing reference point in input coordinate system information.");
        return 0;
    }

    bool canConvertCoords = false;

    switch (this->SourceCoordinateSystem.type.value)
    {
    case CoordinateSystemType::geographic:
    case CoordinateSystemType::metricLocal:
        canConvertCoords = true;
        break;
    case CoordinateSystemType::metricGlobal:
        canConvertCoords = false;
        break;
    case CoordinateSystemType::other:
    default:
        vtkErrorMacro("Unsupported input coordinate system information.");
        return 0;
        break;
    }

    auto outCoordinateSystem = this->SourceCoordinateSystem;
    outCoordinateSystem.type = this->TargetCoordinateSystemType;

    switch (outCoordinateSystem.type.value)
    {
    case CoordinateSystemType::geographic:
    case CoordinateSystemType::metricLocal:
        //canConvertCoords = canConvertCoords && true;
        break;
    case CoordinateSystemType::metricGlobal:
        canConvertCoords = false;
        break;
    case CoordinateSystemType::other:
    default:
        vtkErrorMacro("Unsupported target coordinate system information.");
        return 0;
        break;
    }

    if (!canConvertCoords && (this->SourceCoordinateSystem.type != outCoordinateSystem.type))
    {
        vtkErrorMacro(<< QString("Transformation of coordinates from %1 to %2 not supported.")
            .arg(this->SourceCoordinateSystem.type.toString(), outCoordinateSystem.type.toString()).toUtf8().data());
        return 0;
    }

    auto && unitStr = QString::fromUtf8(this->TargetMetricUnit);

    switch (outCoordinateSystem.type.value)
    {
    case CoordinateSystemType::metricLocal:
    case CoordinateSystemType::metricGlobal:
        if (mathhelper::isValidMetricUnit(unitStr))
        {
            outCoordinateSystem.unitOfMeasurement = unitStr;
        }
        else
        {
            vtkErrorMacro(<< QString("Invalid metric unit: %1").arg(unitStr).toUtf8().data());
            return 0;
        }
        break;
    case CoordinateSystemType::geographic:
    case CoordinateSystemType::other:
    default:
        break;
    }

    this->TargetCoordinateSystem = outCoordinateSystem;
    this->ComputeFilterParameters();

    return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

template<typename Superclass_t>
int AbstractSimpleGeoCoordinateTransformFilter<Superclass_t>::RequestData(
    vtkInformation * /*request*/,
    vtkInformationVector ** /*inputVector*/,
    vtkInformationVector * /*outputVector*/)
{
    return 1;
}

template<typename Superclass_t>
void AbstractSimpleGeoCoordinateTransformFilter<Superclass_t>::ComputeFilterParameters()
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

    static const double earthR_m = 6378138.0;

    const auto La0 = this->TargetCoordinateSystem.referencePointLatLong[1];   // average longitude (Greek lambda)
    const auto Phi0 = this->TargetCoordinateSystem.referencePointLatLong[0];   // average latitude (Greek phi)

    const auto cosPhi0 = std::cos(vtkMath::RadiansFromDegrees(Phi0));
    static const auto toRadians = vtkMath::Pi() / 180.0;
    static const auto toDegrees = 180.0 / vtkMath::Pi();


    vtkVector3d preTranslate = { 0, 0, 0 };
    vtkVector3d scale = { 1, 1, 1 };
    vtkVector3d postTranslate = { 0, 0, 0 };

    double sourceMetricUnitScale = 1.0;
    double targetMetricUnitScale = 1.0;

    // For geographic systems, degrees are assumed as unit.
    if (this->SourceCoordinateSystem.type != CoordinateSystemType::geographic)
    {
        sourceMetricUnitScale =
            mathhelper::scaleFactorForMetricUnits(this->SourceCoordinateSystem.unitOfMeasurement, "m");
    }
    if (this->TargetCoordinateSystem.type != CoordinateSystemType::geographic)
    {
        targetMetricUnitScale =
            mathhelper::scaleFactorForMetricUnits("m", this->TargetCoordinateSystem.unitOfMeasurement);
    }

    if (this->SourceCoordinateSystem.type == this->TargetCoordinateSystem.type)
    {
        const double unitScale = sourceMetricUnitScale * targetMetricUnitScale;
        scale = vtkVector3d{ unitScale, unitScale, 1.0 };
    }
    else if (this->SourceCoordinateSystem.type == CoordinateSystemType::geographic
        && this->TargetCoordinateSystem.type == CoordinateSystemType::metricLocal)
    {
        preTranslate = vtkVector3d{
            -La0,
            -Phi0,
            0.0
        };
        scale = vtkVector3d{
            toRadians * earthR_m * cosPhi0 * targetMetricUnitScale,
            toRadians * earthR_m * targetMetricUnitScale,
            1.0     // do not change elevations
        };
    }
    else if (this->SourceCoordinateSystem.type == CoordinateSystemType::metricLocal
            && this->TargetCoordinateSystem.type == CoordinateSystemType::geographic)
    {
        scale = vtkVector3d{
            sourceMetricUnitScale * toDegrees / (earthR_m * cosPhi0),
            sourceMetricUnitScale * toDegrees / earthR_m,
            1.0
        };
        postTranslate = vtkVector3d{
            La0,
            Phi0,
            0.0
        };
    }

    this->SetFilterParameters(preTranslate, scale, postTranslate);
}


template class AbstractSimpleGeoCoordinateTransformFilter<vtkImageAlgorithm>;
template class AbstractSimpleGeoCoordinateTransformFilter<vtkPolyDataAlgorithm>;
