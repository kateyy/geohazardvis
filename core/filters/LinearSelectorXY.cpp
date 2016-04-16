#include "LinearSelectorXY.h"

#include <cmath>
#include <functional>
#include <memory>
#include <vector>

#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkDemandDrivenPipeline.h>
#include <vtkDoubleArray.h>
#include <vtkExtractSelection.h>
#include <vtkFieldData.h>
#include <vtkIdList.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>
#include <vtkSortDataArray.h>

#include <core/utility/vtkvectorhelper.h>


vtkStandardNewMacro(LinearSelectorXY);


LinearSelectorXY::LinearSelectorXY()
    : Superclass()
    , Sorting{ SortMode::SortPoints }
    , OutputPositionOnLine{ true }
    , ComputeDistanceToLine{ true }
{
    this->SetNumberOfInputPorts(2);
    this->SetNumberOfOutputPorts(2);
}

LinearSelectorXY::~LinearSelectorXY() = default;

vtkPolyData * LinearSelectorXY::GetExtractedPoints()
{
    return static_cast<vtkPolyData *>(this->GetOutputDataObject(1));
}

int LinearSelectorXY::FillInputPortInformation(int port, vtkInformation * info)
{
    if (port == 0)
    {
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
    }
    else
    {
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
    }

    return 1;
}

int LinearSelectorXY::FillOutputPortInformation(int port, vtkInformation * info)
{
    if (port == 1)
    {
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");

        return 1;
    }

    return Superclass::FillOutputPortInformation(port, info);
}

int LinearSelectorXY::RequestData(vtkInformation * vtkNotUsed(request),
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    vtkInformation * inCellsInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation * inCentersInfo = inputVector[1]->GetInformationObject(0);
    vtkInformation * outSelectionInfo = outputVector->GetInformationObject(0);
    vtkInformation * outPointExtractionInfo = outputVector->GetInformationObject(1);

    auto cellInput = vtkPointSet::SafeDownCast(inCellsInfo->Get(vtkDataObject::DATA_OBJECT()));
    auto centersInput = vtkPointSet::SafeDownCast(inCentersInfo->Get(vtkDataObject::DATA_OBJECT()));
    auto selectionOutput = vtkSelection::SafeDownCast(outSelectionInfo->Get(vtkDataObject::DATA_OBJECT()));
    auto extractedOutput = vtkPolyData::SafeDownCast(outPointExtractionInfo->Get(vtkDataObject::DATA_OBJECT()));

    auto inputCellData = cellInput->GetCellData();
    auto extractionPointData = extractedOutput->GetPointData();

    const vtkIdType numInputPoints = cellInput->GetNumberOfPoints();
    const vtkIdType numInputCells = cellInput->GetNumberOfCells();

    if (numInputPoints == 0)
    {
        vtkWarningMacro(<< "LinearSelectorXY: No points in input.");
        return 1;
    }
    if (numInputCells == 0)
    {
        vtkWarningMacro(<< "LinearSelectorXY: No cells in input.");
        return 1;
    }

    if (numInputCells != centersInput->GetNumberOfPoints())
    {
        vtkErrorMacro(<< "LinearSelectorXY: Number of input cell centers does not match number of input cells.");
        return 0;
    }

    const auto & A = this->StartPoint;
    const auto & B = this->EndPoint;
    const auto AB = B - A;
    const double ABnorm2Inv = 1.0 / AB.SquaredNorm();
    const auto ABnormal = vtkVector2d{ AB[1], -AB[0] }.Normalized();

    auto signedDistanceToLine = [A, ABnormal] (const vtkVector2d & P) -> double
    {
        return (P - A).Dot(ABnormal);
    };

    auto positionOnLine = [A, AB, ABnorm2Inv] (const vtkVector2d & P) -> double
    {
        return (P - A).Dot(AB) * ABnorm2Inv;
    };


    // compute relative position in half spaces for all points
    std::vector<double> signedPointDistances(static_cast<size_t>(numInputPoints));
    for (vtkIdType i = 0; i < numInputPoints; ++i)
    {
        double point[3];
        cellInput->GetPoint(i, point);

        signedPointDistances[i] = signedDistanceToLine({ point[0], point[1] });
    }


    auto selectedCells = vtkSmartPointer<vtkIdTypeArray>::New();
    selectedCells->SetName("OriginalCellIds");

    auto positionsOnLine = vtkSmartPointer<vtkDoubleArray>::New();
    positionsOnLine->SetName("positionOnLine");
    
    vtkSmartPointer<vtkDoubleArray> distanceToLine;
    if (ComputeDistanceToLine)
    {
        distanceToLine = vtkSmartPointer<vtkDoubleArray>::New();
        distanceToLine->SetName("distanceToLine");
    }


    auto extractedPoints = vtkSmartPointer<vtkPoints>::New();
    std::vector<vtkIdType> extractedPointIds;

    auto pointIdList = vtkSmartPointer<vtkIdList>::New();
    for (vtkIdType cellId = 0; cellId < numInputCells; ++cellId)
    {
        // check if the line intersects the cell: there must be points on both sides of the line

        cellInput->GetCellPoints(cellId, pointIdList);
        vtkIdType pointId = pointIdList->GetId(0);

        bool lastHalfSpace = signedPointDistances[pointId] >= 0;
        bool pointsInBothHalfSpaces = false;

        for (vtkIdType cpId = 1; !pointsInBothHalfSpaces && cpId < pointIdList->GetNumberOfIds(); ++cpId)
        {
            const bool halfSpace = signedPointDistances[pointIdList->GetId(cpId)] >= 0;
            pointsInBothHalfSpaces = pointsInBothHalfSpaces || (lastHalfSpace != halfSpace);
            lastHalfSpace = halfSpace;
        }

        if (!pointsInBothHalfSpaces)
        {
            continue;
        }


        // check if the centroid lies within the line segment

        const double * centroid = centersInput->GetPoint(cellId);
        vtkVector2d centroid2d = { centroid[0], centroid[1] };

        const double t = positionOnLine(centroid2d);

        if (t < 0 || t > 1)
        {
            continue;
        }

        selectedCells->InsertNextValue(cellId);

        extractedPointIds.push_back(
            extractedPoints->InsertNextPoint(centroid));

        positionsOnLine->InsertNextValue(t);
        
        if (ComputeDistanceToLine)
        {
            distanceToLine->InsertNextValue(std::abs(signedDistanceToLine(centroid2d)));
        }
    }

    const vtkIdType outputNumPoints = selectedCells->GetNumberOfTuples();

    // Port 0: create selection information
    auto selectionNode = vtkSmartPointer<vtkSelectionNode>::New();
    selectionNode->SetFieldType(vtkSelectionNode::CELL);
    selectionNode->SetContentType(vtkSelectionNode::INDICES);
    selectionNode->SetSelectionList(selectedCells);
    selectionOutput->AddNode(selectionNode);


    // Port 1: create extracted point output

    // Sort output point w.r.t to the orientation of the line segment.

    std::unique_ptr<vtkIdType> sortIndices;

    if (this->Sorting == SortMode::SortIndices)
    {
        vtkSortDataArray::GenerateSortIndices(
            positionsOnLine->GetDataType(),
            positionsOnLine->GetVoidPointer(0),
            outputNumPoints,
            positionsOnLine->GetNumberOfComponents(),
            0,
            extractedPointIds.data());
    }
    else if (this->Sorting == SortMode::SortPoints)
    {
        sortIndices.reset(vtkSortDataArray::InitializeSortIndices(outputNumPoints));
        vtkSortDataArray::GenerateSortIndices(
            positionsOnLine->GetDataType(),
            positionsOnLine->GetVoidPointer(0),
            outputNumPoints,
            positionsOnLine->GetNumberOfComponents(),
            0,
            sortIndices.get());

        vtkSortDataArray::ShuffleArray(
            sortIndices.get(),
            extractedPoints->GetDataType(),
            outputNumPoints,
            extractedPoints->GetData()->GetNumberOfComponents(),
            extractedPoints->GetData(),
            extractedPoints->GetData()->GetVoidPointer(0),
            0);
    }

    // pass input cell data to output points (centroids)
    extractionPointData->CopyAllocate(inputCellData, outputNumPoints);

    std::function<vtkIdType(vtkIdType)> idGetter;
    if (this->Sorting == SortMode::SortPoints)
    {
        // need to reorder all attributes
        idGetter = [&sortIndices] (vtkIdType i) -> vtkIdType { 
            return sortIndices.get()[i]; 
        };
    }
    else
    {
        // keep the ordering
        idGetter = std::identity<vtkIdType>();
    }

    for (vtkIdType outputPointId = 0; outputPointId < outputNumPoints; ++outputPointId)
    {
        const vtkIdType beforeReorderPointId = idGetter(outputPointId);
        const vtkIdType inputCellId = selectedCells->GetValue(beforeReorderPointId);

        extractionPointData->CopyData(
            inputCellData, inputCellId,
            outputPointId);
    }

    auto verts = vtkSmartPointer<vtkCellArray>::New();
    verts->InsertNextCell(extractedPointIds.size(), extractedPointIds.data());

    extractedOutput->SetPoints(extractedPoints);
    extractedOutput->SetVerts(verts);

    if (OutputPositionOnLine)
    {
        extractionPointData->AddArray(positionsOnLine);
    }

    if (ComputeDistanceToLine)
    {
        if (this->Sorting == SortMode::SortPoints)
        {
            vtkSortDataArray::ShuffleArray(
                sortIndices.get(),
                distanceToLine->GetDataType(),
                outputNumPoints,
                distanceToLine->GetNumberOfComponents(),
                distanceToLine,
                distanceToLine->GetVoidPointer(0),
                0);
        }
        extractionPointData->AddArray(distanceToLine);
    }

    return 1;
}
