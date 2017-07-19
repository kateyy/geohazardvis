/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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