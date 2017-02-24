#include <core/filters/SimpleImageGeoCoordinateTransformFilter.h>

#include <vtkDataObject.h>
#include <vtkImageChangeInformation.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkInformationIntegerVectorKey.h>
#include <vtkObjectFactory.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkVector.h>

#include <core/filters/SetCoordinateSystemInformationFilter.h>
#include <core/utility/DataExtent.h>


vtkStandardNewMacro(SimpleImageGeoCoordinateTransformFilter);


SimpleImageGeoCoordinateTransformFilter::SimpleImageGeoCoordinateTransformFilter()
    : Superclass()
    , Step1{ vtkSmartPointer<vtkImageChangeInformation>::New() }
    , Step2{ vtkSmartPointer<vtkImageChangeInformation>::New() }
    , InfoSetter{ vtkSmartPointer<SetCoordinateSystemInformationFilter>::New() }
{
    this->Step2->SetInputConnection(this->Step1->GetOutputPort());
    this->InfoSetter->SetInputConnection(this->Step2->GetOutputPort());
}

SimpleImageGeoCoordinateTransformFilter::~SimpleImageGeoCoordinateTransformFilter() = default;

int SimpleImageGeoCoordinateTransformFilter::RequestDataObject(
    vtkInformation * /*request*/,
    vtkInformationVector ** /*inputVector*/,
    vtkInformationVector * outputVector)
{
    this->InfoSetter->UpdateDataObject();

    outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(),
        this->InfoSetter->GetOutputDataObject(0));

    return 1;
}

int SimpleImageGeoCoordinateTransformFilter::RequestInformation(vtkInformation * request, 
    vtkInformationVector ** inputVector, vtkInformationVector * outputVector)
{
    if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
    {
        return 0;
    }

    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto inData = inInfo->Get(vtkDataObject::DATA_OBJECT());

    this->Step1->SetInputData(inData);
    // Update the trivial producer now, so that it won't discard the input information manually
    // set below.
    this->Step1->GetInputAlgorithm()->Update();

    // Pass available input information to the internal pipeline
    ImageExtent inExtent;
    vtkVector3d inOrigin, inSpacing;
    if (inInfo->Has(vtkDataObject::ORIGIN()))
    {
        inInfo->Get(vtkDataObject::ORIGIN(), inOrigin.GetData());
        this->Step1->GetInputInformation()->Set(vtkDataObject::ORIGIN(),
            inOrigin.GetData(), inOrigin.GetSize());
    }
    else
    {
        this->Step1->GetInputInformation()->Remove(vtkDataObject::ORIGIN());
    }
    if (inInfo->Has(vtkDataObject::SPACING()))
    {
        inInfo->Get(vtkDataObject::SPACING(), inSpacing.GetData());
        this->Step1->GetInputInformation()->Set(vtkDataObject::SPACING(),
            inSpacing.GetData(), inSpacing.GetSize());
    }
    else
    {
        this->Step1->GetInputInformation()->Remove(vtkDataObject::SPACING());
    }
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
        inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExtent.data());
        this->Step1->GetInputInformation()->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
            inExtent.data(), inExtent.ValueCount);
    }
    else
    {
        this->Step1->GetInputInformation()->Remove(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    }

    // Make sure the algorithms process the new input information
    this->Step1->Modified();

    this->InfoSetter->SetCoordinateSystemSpec(this->TargetCoordinateSystem);

    if (!this->InfoSetter->GetExecutive()->UpdateInformation())
    {
        vtkErrorMacro("Error occurred while updating transform information.");
        return 0;
    }

    auto internalOutInfo = this->InfoSetter->GetOutputInformation(0);
    auto outInfo = outputVector->GetInformationObject(0);

    // overwrite information changed values.

    vtkVector3d newSpacing, newOrigin;
    internalOutInfo->Get(vtkDataObject::SPACING(), newSpacing.GetData());
    internalOutInfo->Get(vtkDataObject::ORIGIN(), newOrigin.GetData());

    outInfo->Set(vtkDataObject::SPACING(), newSpacing.GetData(), newSpacing.GetSize());
    outInfo->Set(vtkDataObject::ORIGIN(), newOrigin.GetData(), newOrigin.GetSize());

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()->Length(inInfo));

    return 1;
}

int SimpleImageGeoCoordinateTransformFilter::RequestData(vtkInformation * /*request*/,
    vtkInformationVector ** /*inputVector*/, vtkInformationVector * /*outputVector*/)
{
    return this->InfoSetter->GetExecutive()->Update();
}

int SimpleImageGeoCoordinateTransformFilter::FillInputPortInformation(int /*port*/, vtkInformation * info)
{
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");

    return 1;
}

int SimpleImageGeoCoordinateTransformFilter::FillOutputPortInformation(int /*port*/, vtkInformation * info)
{
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");

    return 1;
}

void SimpleImageGeoCoordinateTransformFilter::SetFilterParameters(
    const vtkVector3d & constPreTranslate,
    const vtkVector3d & constScale,
    const vtkVector3d & constPostTranslate)
{
    auto preTranslate = constPreTranslate;
    auto scale = constScale;
    auto postTranslate = constPostTranslate;

    this->Step1->SetOriginTranslation(preTranslate.GetData());
    this->Step2->SetOriginScale(scale.GetData());
    this->Step2->SetSpacingScale(scale.GetData());
    this->Step2->SetOriginTranslation(postTranslate.GetData());
}
