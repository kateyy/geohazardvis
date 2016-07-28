#include "DEMToTopographyMesh.h"

#include <vtkDemandDrivenPipeline.h>
#include <vtkImageData.h>
#include <vtkImageShiftScale.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkProbeFilter.h>
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

bool DEMToTopographyMesh::CenterTopoMesh()
{
    auto dem = this->CheckUpdateInputDEM();
    if (!dem)
    {
        return false;
    }

    const auto demCenter = convert<2>(DataBounds(dem->GetBounds()).center());
    this->SetTopographyShiftXY(demCenter);

    return true;
}

bool DEMToTopographyMesh::MatchTopoMeshRadius()
{
    auto dem = this->CheckUpdateInputDEM();
    if (!dem)
    {
        return false;
    }

    bool isValid = false;
    const auto radiusRange = this->ComputeValidRadiusRange(&isValid);
    if (!isValid)
    {
        return false;
    }

    this->SetTopographyRadius(radiusRange[1]);

    return true;
}

bool DEMToTopographyMesh::SetParametersToMatching()
{
    if (!CenterTopoMesh())
    {
        return false;
    }

    return MatchTopoMeshRadius();
}

DataExtent<double, 2> DEMToTopographyMesh::ComputeValidShiftRange(bool * isValid)
{
    if (isValid)
    {
        *isValid = false;
    }

    auto xyRange = decltype(DEMToTopographyMesh::ComputeValidShiftRange()){};

    auto dem = this->CheckUpdateInputDEM();

    if (!dem)
    {
        return xyRange;
    }

    const auto currentRadius = this->TopographyRadius;

    const auto demBounds = DataBounds(dem->GetBounds()).convertTo<2>();
    const auto demCenter = demBounds.center();

    // clamp valid mesh center to the DEM bounds reduced by current radius
    const auto mins = min(demBounds.minRange() + currentRadius, demCenter);
    const auto maxs = max(demBounds.maxRange() - currentRadius, demCenter);

    xyRange.setDimension(0, vtkVector2d{ mins[0], maxs[0] });
    xyRange.setDimension(1, vtkVector2d{ mins[1], maxs[1] });

    if (isValid)
    {
        *isValid = true;
    }

    return xyRange;
}

DataExtent<double, 1> DEMToTopographyMesh::ComputeValidRadiusRange(bool * isValid)
{
    if (isValid)
    {
        *isValid = false;
    }

    auto range = decltype(DEMToTopographyMesh::ComputeValidRadiusRange()){};

    auto dem = this->CheckUpdateInputDEM();
    if (!dem)
    {
        return range;
    }

    const auto demBounds = DataBounds(dem->GetBounds()).convertTo<2>();
    if (!demBounds.contains(this->TopographyShiftXY))
    {
        return range;
    }

    if (isValid)
    {
        *isValid = true;
    }

    // use minimum of x- and y-spacing as sensible minimal radius
    range[0] = minComponent(vtkVector2d(dem->GetSpacing()));

    // clamp from topography center to the nearest DEM border
    range[1] = std::min(
        minComponent(abs(this->TopographyShiftXY - demBounds.minRange())),
        minComponent(abs(this->TopographyShiftXY - demBounds.maxRange())));

    return range;
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

    this->WarpElevation->UpdateDataObject();

    outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(),
        this->CenterOutputMeshFilter->GetOutput());

    outputVector->GetInformationObject(1)->Set(vtkDataObject::DATA_OBJECT(),
        this->WarpElevation->GetOutput());

    return 1;
}

int DEMToTopographyMesh::RequestInformation(
    vtkInformation * /*request*/,
    vtkInformationVector ** /*inputVector*/,
    vtkInformationVector * /*outputVector*/)
{
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

vtkImageData * DEMToTopographyMesh::CheckUpdateInputDEM()
{
    auto demInput = this->GetInputAlgorithm(0, 0);
    if (!demInput)
    {
        vtkWarningMacro("Missing input DEM.");
        return nullptr;
    }

    demInput->Update();

    auto dem = vtkImageData::SafeDownCast(this->GetInputDataObject(0, 0));

    if (!dem)
    {
        vtkWarningMacro("Invalid input DEM.");
        return nullptr;
    }

    return dem;
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
