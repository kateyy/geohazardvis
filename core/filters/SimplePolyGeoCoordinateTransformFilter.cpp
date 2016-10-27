#include <core/filters/SimplePolyGeoCoordinateTransformFilter.h>

#include <cassert>

#include <vtkExecutive.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVector.h>

#include <core/filters/SetCoordinateSystemInformationFilter.h>


vtkStandardNewMacro(SimplePolyGeoCoordinateTransformFilter);


SimplePolyGeoCoordinateTransformFilter::SimplePolyGeoCoordinateTransformFilter()
    : Superclass()
    , TransformFilter{ vtkSmartPointer<vtkTransformPolyDataFilter>::New() }
    , Transform{ vtkSmartPointer<vtkTransform>::New() }
    , InfoSetter{ vtkSmartPointer<SetCoordinateSystemInformationFilter>::New() }
{
    this->Transform->PostMultiply();
    this->TransformFilter->SetTransform(Transform);
    this->InfoSetter->SetInputConnection(this->TransformFilter->GetOutputPort());
}

SimplePolyGeoCoordinateTransformFilter::~SimplePolyGeoCoordinateTransformFilter() = default;

int SimplePolyGeoCoordinateTransformFilter::RequestDataObject(
    vtkInformation * /*request*/,
    vtkInformationVector ** /*inputVector*/,
    vtkInformationVector * outputVector)
{
    this->InfoSetter->UpdateDataObject();

    outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(),
        this->InfoSetter->GetOutput());

    return 1;
}

int SimplePolyGeoCoordinateTransformFilter::RequestInformation(
    vtkInformation * request, 
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
    {
        return 0;
    }

    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto inData = inInfo->Get(vtkDataObject::DATA_OBJECT());

    this->TransformFilter->SetInputData(inData);

    this->InfoSetter->SetCoordinateSystemSpec(this->TargetCoordinateSystem);

    if (!this->InfoSetter->GetExecutive()->UpdateInformation())
    {
        vtkErrorMacro("Error occurred while updating transform information.");
        return 0;
    }

    return 1;
}

int SimplePolyGeoCoordinateTransformFilter::RequestData(vtkInformation * /*request*/,
    vtkInformationVector ** /*inputVector*/, vtkInformationVector * /*outputVector*/)
{
    return this->InfoSetter->GetExecutive()->Update();
}

int SimplePolyGeoCoordinateTransformFilter::FillInputPortInformation(int /*port*/, vtkInformation * info)
{
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");

    return 1;
}

int SimplePolyGeoCoordinateTransformFilter::FillOutputPortInformation(int /*port*/, vtkInformation * info)
{
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");

    return 1;
}

void SimplePolyGeoCoordinateTransformFilter::SetFilterParameters(
    const vtkVector3d & preTranslate,
    const vtkVector3d & scale,
    const vtkVector3d & postTranslate)
{
    this->Transform->Identity();
    this->Transform->Translate(preTranslate.GetData());
    this->Transform->Scale(scale.GetData());
    this->Transform->Translate(postTranslate.GetData());
}
