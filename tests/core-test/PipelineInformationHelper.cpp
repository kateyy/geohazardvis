#include "PipelineInformationHelper.h"

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkDemandDrivenPipeline.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>


vtkStandardNewMacro(InformationSource);


void InformationSource::SetOutInfo(vtkInformation * info)
{
    this->OutInfo = info;
    this->Modified();
}

vtkInformation * InformationSource::GetOutInfo()
{
    if (!this->OutInfo)
    {
        this->OutInfo = vtkSmartPointer<vtkInformation>::New();
    }

    return this->OutInfo;
}

InformationSource::InformationSource()
    : Superclass()
{
}

InformationSource::~InformationSource() = default;

int InformationSource::ProcessRequest(vtkInformation * request,
    vtkInformationVector ** inVector, vtkInformationVector * outVector)
{
    if (!this->Superclass::ProcessRequest(request, inVector, outVector))
    {
        return 0;
    }

    if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
        return RequestInformation(request, inVector, outVector);
    }

    return 1;
}

int InformationSource::RequestInformation(vtkInformation * /*request*/,
    vtkInformationVector ** /*inVector*/, vtkInformationVector * outVector)
{
    if (!this->OutInfo)
    {
        return 1;
    }

    auto outInfo = outVector->GetInformationObject(0);

    outInfo->Append(this->OutInfo, 1);

    return 1;
}


vtkStandardNewMacro(InformationSink);


vtkInformation * InformationSink::GetInInfo()
{
    return this->InInfo;
}

InformationSink::InformationSink()
    : Superclass()
    , InInfo{ vtkSmartPointer<vtkInformation>::New() }
{
    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(0);
}

InformationSink::~InformationSink() = default;

int InformationSink::FillInputPortInformation(int /*port*/, vtkInformation * /*info*/)
{
    return 1;
}

int InformationSink::ProcessRequest(vtkInformation * request,
    vtkInformationVector ** inVector, vtkInformationVector * outVector)
{
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
        return RequestInformation(request, inVector, outVector);
    }

    return Superclass::ProcessRequest(request, inVector, outVector);
}

int InformationSink::RequestInformation(vtkInformation * /*request*/,
    vtkInformationVector ** inVector, vtkInformationVector * /*outVector*/)
{
    auto inInfo = inVector[0]->GetInformationObject(0);

    this->InInfo->Copy(inInfo, 1);

    return 1;
}