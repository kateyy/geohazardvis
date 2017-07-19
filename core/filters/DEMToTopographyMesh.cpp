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

#include "DEMToTopographyMesh.h"

#include <vtkImageData.h>
#include <vtkImageShiftScale.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkProbeFilter.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkWarpScalar.h>

#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>


vtkStandardNewMacro(DEMToTopographyMesh)


DEMToTopographyMesh::DEMToTopographyMesh()
    : Superclass()
    , ElevationScaleFactor{ 1.0 }
    , TopographyRadius{ 1.0 }
    , TopographyShiftXY{ 0.0 }
{
    this->SetNumberOfInputPorts(2);
    this->SetNumberOfOutputPorts(2);
}

DEMToTopographyMesh::~DEMToTopographyMesh() = default;

void DEMToTopographyMesh::CenterTopoMesh()
{
    this->SetTopographyShiftXY(GetValidShiftRange().center());
}

void DEMToTopographyMesh::MatchTopoMeshRadius()
{
    this->SetTopographyRadius(GetValidRadiusRange().max());
}

void DEMToTopographyMesh::SetParametersToMatching()
{
    CenterTopoMesh();
    MatchTopoMeshRadius();
}

const DataExtent<double, 2> & DEMToTopographyMesh::GetValidShiftRange()
{
    this->GetExecutive()->UpdateInformation();

    return this->ValidShiftRange;
}

const DataExtent<double, 1> & DEMToTopographyMesh::GetValidRadiusRange()
{
    this->GetExecutive()->UpdateInformation();

    return this->ValidRadiusRange;
}

int DEMToTopographyMesh::FillOutputPortInformation(int port, vtkInformation * info)
{
    switch (port)
    {
    case 0:
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
        break;
    case 1:
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
        break;
    default:
        return 0;
    }

    return 1;
}

int DEMToTopographyMesh::FillInputPortInformation(int port, vtkInformation * info)
{
    switch (port)
    {
    case 0:
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
        break;
    case 1:
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
        break;
    default:
        return 0;
    }

    return 1;
}

int DEMToTopographyMesh::ProcessRequest(
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

    return Superclass::ProcessRequest(request, inputVector, outputVector);
}

int DEMToTopographyMesh::RequestDataObject(vtkInformation * /*request*/,
    vtkInformationVector ** /*inputVector*/,
    vtkInformationVector * outputVector)
{
    this->SetupPipeline();

    this->CenterOutputMeshFilter->UpdateDataObject();

    outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(),
        this->CenterOutputMeshFilter->GetOutput());

    outputVector->GetInformationObject(1)->Set(vtkDataObject::DATA_OBJECT(),
        this->WarpElevation->GetOutput());

    return 1;
}

int DEMToTopographyMesh::RequestInformation(
    vtkInformation * /*request*/,
    vtkInformationVector ** inputVector,
    vtkInformationVector * /*outputVector*/)
{
    this->ValidShiftRange = {};

    auto inInfo = inputVector[0]->GetInformationObject(0);

    DataBounds demBounds3d;
    if (inInfo->Has(vtkDataObject::BOUNDING_BOX()))
    {
        inInfo->Get(vtkDataObject::BOUNDING_BOX(), demBounds3d.data());
    }
    else if (inInfo->Has(vtkStreamingDemandDrivenPipeline::BOUNDS()))
    {
        inInfo->Get(vtkStreamingDemandDrivenPipeline::BOUNDS(), demBounds3d.data());
    }
    else if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()) 
        && inInfo->Has(vtkDataObject::SPACING()) && inInfo->Has(vtkDataObject::ORIGIN()))
    {
        auto boundsCheck = vtkSmartPointer<vtkImageData>::New();
        boundsCheck->SetExtent(inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
        boundsCheck->SetSpacing(inInfo->Get(vtkDataObject::SPACING()));
        boundsCheck->SetOrigin(inInfo->Get(vtkDataObject::ORIGIN()));

        boundsCheck->GetBounds(demBounds3d.data());
    }
    else
    {
        // TODO fall back to information setup in RequestData() ?
        return 0;
    }

    if (!inInfo->Has(vtkDataObject::SPACING()))
    {
        return 0;
    }

    const auto currentRadius = this->TopographyRadius;

    const auto demBounds = demBounds3d.convertTo<2>();
    const auto demCenter = demBounds.center();

    // clamp valid mesh center to the DEM bounds reduced by current radius
    const auto mins = min(demBounds.min() + currentRadius, demCenter);
    const auto maxs = max(demBounds.max() - currentRadius, demCenter);

    this->ValidShiftRange.setDimension(0, mins[0], maxs[0]);
    this->ValidShiftRange.setDimension(1, mins[1], maxs[1]);


    vtkVector3d demSpacing;
    inInfo->Get(vtkDataObject::SPACING(), demSpacing.GetData());

    // use minimum of x- and y-spacing as sensible minimal radius
    this->ValidRadiusRange[0] = minComponent(convertTo<2>(demSpacing));

    if (demBounds.contains(this->TopographyShiftXY))
    {
        // clamp from topography center to the nearest DEM border
        this->ValidRadiusRange[1] = std::min(
            minComponent(abs(this->TopographyShiftXY - demBounds.min())),
            minComponent(abs(this->TopographyShiftXY - demBounds.max())));
    }
    else
    {
        // if current shift is invalid, also set the radius range to invalid
        this->ValidRadiusRange[1] = 0.0;
    }

    return 1;
}

int DEMToTopographyMesh::RequestData(
    vtkInformation * /*request*/,
    vtkInformationVector ** inputVector,
    vtkInformationVector * /*outputVector*/)
{
    auto inDEMInfo = inputVector[0]->GetInformationObject(0);
    auto inMeshInfo = inputVector[1]->GetInformationObject(0);

    auto dem = vtkImageData::SafeDownCast(inDEMInfo->Get(vtkDataObject::DATA_OBJECT()));
    auto meshTemplate = vtkPolyData::SafeDownCast(inMeshInfo->Get(vtkDataObject::DATA_OBJECT()));


    this->DEMScaleElevationFilter->SetInputData(dem);
    this->MeshTransformFilter->SetInputData(meshTemplate);


    const auto meshTemplateRadius = ComputeCenteredMeshRadius(*meshTemplate);
    if (meshTemplateRadius <= std::numeric_limits<decltype(meshTemplateRadius)>::epsilon())
    {
        vtkWarningMacro("Invalid or empty input mesh template.");
        return 0;
    }

    const auto radiusScale = this->TopographyRadius / meshTemplateRadius;

    this->DEMScaleElevationFilter->SetScale(this->ElevationScaleFactor);

    this->MeshTransform->Identity();
    this->MeshTransform->PostMultiply();
    this->MeshTransform->Scale(radiusScale, radiusScale, 0.0);
    // assuming the template is already centered around (0,0,z):
    this->MeshTransform->Translate(convertTo<3>(this->TopographyShiftXY, 0.0).GetData());

    this->CenterOutputMeshTransform->Identity();
    this->CenterOutputMeshTransform->Translate(convertTo<3>(-this->TopographyShiftXY, 0.0).GetData());

    return this->CenterOutputMeshFilter->GetExecutive()->Update();

    // output's are already mapped to the internal filters' outputs
}

double DEMToTopographyMesh::ComputeCenteredMeshRadius(vtkPolyData & mesh)
{
    auto & points = *mesh.GetPoints();
    const auto numPoints = points.GetNumberOfPoints();

    vtkVector3d point;
    double maxDistance = 0.0;

    // assume a template centered around (0, 0, z)
    for (vtkIdType i = 0; i < numPoints; ++i)
    {
        points.GetPoint(i, point.GetData());
        maxDistance = std::max(maxDistance, convertTo<2>(point).Norm());
    }

    return maxDistance;
}

void DEMToTopographyMesh::SetupPipeline()
{
    if (this->MeshTransform)
    {
        return;
    }

    this->DEMScaleElevationFilter = vtkSmartPointer<vtkImageShiftScale>::New();

    this->MeshTransform = vtkSmartPointer<vtkTransform>::New();
    this->MeshTransformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    this->MeshTransformFilter->SetTransform(this->MeshTransform);

    auto probe = vtkSmartPointer<vtkProbeFilter>::New();
    probe->SetInputConnection(this->MeshTransformFilter->GetOutputPort());
    probe->SetSourceConnection(this->DEMScaleElevationFilter->GetOutputPort());

    this->WarpElevation = vtkSmartPointer<vtkWarpScalar>::New();
    this->WarpElevation->SetInputConnection(probe->GetOutputPort());

    this->CenterOutputMeshTransform = vtkSmartPointer<vtkTransform>::New();
    this->CenterOutputMeshFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    this->CenterOutputMeshFilter->SetTransform(this->CenterOutputMeshTransform);
    this->CenterOutputMeshFilter->SetInputConnection(this->WarpElevation->GetOutputPort());
}

void DEMToTopographyMesh::SetInputDEM(vtkImageData * dem)
{
    this->SetInputDataInternal(0, dem);
}

void DEMToTopographyMesh::SetInputMeshTemplate(vtkPolyData * meshTemplate)
{
    this->SetInputDataInternal(1, meshTemplate);
}

vtkPolyData * DEMToTopographyMesh::GetOutputTopography()
{
    return vtkPolyData::SafeDownCast(this->GetOutputDataObject(0));
}

vtkAlgorithmOutput * DEMToTopographyMesh::GetTopographyOutputPort()
{
    return this->GetOutputPort(0);
}

vtkPolyData * DEMToTopographyMesh::GetOutputTopoMeshOnDEM()
{
    return vtkPolyData::SafeDownCast(this->GetOutputDataObject(1));
}

vtkAlgorithmOutput * DEMToTopographyMesh::GetTopoMeshOnDEMOutputPort()
{
    return this->GetOutputPort(1);
}

const vtkVector2d & DEMToTopographyMesh::GetTopographyShiftXY()
{
    // vtkGetMacro returns by value, not by const&
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << "TopographyShiftXY of " << this->TopographyShiftXY);
    return this->TopographyShiftXY;
}
