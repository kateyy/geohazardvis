#include "LineOnPointsSelector2D.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <vector>

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
#include <vtkStaticPointLocator.h>


#include <core/utility/vtkvectorhelper.h>


vtkStandardNewMacro(LineOnPointsSelector2D);


LineOnPointsSelector2D::LineOnPointsSelector2D()
    : Superclass()
    , Sorting{ SortMode::SortPoints }
    , PassPositionOnLine{ true }
    , PassDistanceToLine{ true }
    , InputPointsMTime{}
    , ApproxGridSpacing{ 0.0 }
    , GridSpacingStandardDeviation{ 0.0 }
{
    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(2);
}

LineOnPointsSelector2D::~LineOnPointsSelector2D() = default;

vtkPolyData * LineOnPointsSelector2D::GetExtractedPoints()
{
    return static_cast<vtkPolyData *>(this->GetOutputDataObject(1));
}

int LineOnPointsSelector2D::FillInputPortInformation(int port, vtkInformation * info)
{
    if (port == 0)
    {
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
    }

    return 1;
}

int LineOnPointsSelector2D::FillOutputPortInformation(int port, vtkInformation * info)
{
    if (port == 1)
    {
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");

        return 1;
    }

    return Superclass::FillOutputPortInformation(port, info);
}

int LineOnPointsSelector2D::RequestData(vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    const int requestedPort = request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());
    const bool generateSelection = requestedPort == 0;
    const bool generateGeometry = requestedPort == 1;

    vtkInformation * inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation * outSelectionInfo = outputVector->GetInformationObject(0);
    vtkInformation * outPointExtractionInfo = outputVector->GetInformationObject(1);

    auto inputPointSet = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    auto selectionOutput = vtkSelection::SafeDownCast(outSelectionInfo->Get(vtkDataObject::DATA_OBJECT()));
    auto extractedOutput = vtkPolyData::SafeDownCast(outPointExtractionInfo->Get(vtkDataObject::DATA_OBJECT()));

    auto inputPointData = inputPointSet->GetPointData();
    auto extractionPointData = extractedOutput->GetPointData();

    const vtkIdType numInputPoints = inputPointSet->GetNumberOfPoints();
    if (numInputPoints == 0)
    {
        vtkWarningMacro(<< "LineOnPointsSelector2D: No points in input.");
        return 1;
    }

    auto & inputPoints = *inputPointSet->GetPoints();

    const auto pointsMTime = std::max(inputPointSet->GetMTime(), inputPoints.GetMTime());

    // Approximate grid spacing
    if (this->InputPointsMTime < pointsMTime)
    {
        this->InputPointsMTime = pointsMTime;
        auto locator = vtkSmartPointer<vtkStaticPointLocator>::New();
        locator->AutomaticOn();
        locator->SetDataSet(inputPointSet);
        locator->BuildLocator();

        auto neighbors = vtkSmartPointer<vtkIdList>::New();
        neighbors->SetNumberOfIds(2);
        vtkVector3d point, neighbor;
        double meanDistance = 0.0;
        const double inv_numInputPoints = 1.0 / static_cast<double>(numInputPoints);

        auto neighborhoodDistances = std::vector<double>(static_cast<size_t>(numInputPoints));

        auto & points = *inputPointSet->GetPoints();
        for (vtkIdType i = 0; i < numInputPoints; ++i)
        {
            points.GetPoint(i, point.GetData());
            locator->FindClosestNPoints(2, point.GetData(), neighbors);
            assert(neighbors->GetNumberOfIds() == 2);
            assert(point == vtkVector3d(points.GetPoint(neighbors->GetId(0))));

            points.GetPoint(neighbors->GetId(1), neighbor.GetData());
            const double distance = (point - neighbor).Norm();
            meanDistance += distance * inv_numInputPoints;
            neighborhoodDistances[static_cast<size_t>(i)] = distance;
        }

        this->GridSpacingStandardDeviation = std::sqrt(std::accumulate(
            neighborhoodDistances.begin(), neighborhoodDistances.end(), 0.0,
            [meanDistance, inv_numInputPoints] (const double lastResult, double d)
        {
            d = d - meanDistance;
            return lastResult + inv_numInputPoints * d * d;
        }));

        // Search for points that have neighbors in an assumed grid.
        // Distances shouldn't variate too much and the minimum distance should be near the
        // grid spacing.

        const size_t numPointsToConsider = std::max(size_t(1u), neighborhoodDistances.size() / 2u);
        const auto midIt = neighborhoodDistances.begin() + numPointsToConsider;
        std::partial_sort(neighborhoodDistances.begin(), midIt, neighborhoodDistances.end());

        this->ApproxGridSpacing = std::accumulate(neighborhoodDistances.begin(), midIt, 0.0)
            / static_cast<double>(numPointsToConsider);
    }


    const auto & A = this->StartPoint;
    const auto & B = this->EndPoint;
    const auto AB = B - A;
    const double ABnorm2Inv = 1.0 / AB.SquaredNorm();
    const auto ABnormal = vtkVector2d{ AB[1], -AB[0] }.Normalized();

    auto distanceToLine = [A, ABnormal] (const vtkVector2d & P) -> double
    {
        return std::abs((P - A).Dot(ABnormal));
    };

    auto positionOnLine = [A, AB, ABnorm2Inv] (const vtkVector2d & P) -> double
    {
        return (P - A).Dot(AB) * ABnorm2Inv;
    };


    auto selectedPoints = vtkSmartPointer<vtkIdTypeArray>::New();
    selectedPoints->SetName("OriginalPointIds");

    const bool computePositionOnLine = generateGeometry
        && (this->PassPositionOnLine || this->Sorting != SortNone);

    vtkSmartPointer<vtkDoubleArray> positionsOnLine;
    if (computePositionOnLine)
    {
        positionsOnLine = vtkSmartPointer<vtkDoubleArray>::New();
        positionsOnLine->SetName("PositionOnLine");
    }

    vtkSmartPointer<vtkDoubleArray> distancesToLine;
    if (this->PassDistanceToLine)
    {
        distancesToLine = vtkSmartPointer<vtkDoubleArray>::New();
        distancesToLine->SetName("DistanceToLine");
    }

    const double maxAllowedDistance =
        0.5 * this->ApproxGridSpacing + this->GridSpacingStandardDeviation;

    auto extractedPoints = vtkSmartPointer<vtkPoints>::New();
    std::vector<vtkIdType> extractedPointIds;

    auto & pointCoords = *inputPoints.GetData();

    auto pointIdList = vtkSmartPointer<vtkIdList>::New();
    for (vtkIdType pointId = 0; pointId < numInputPoints; ++pointId)
    {
        vtkVector3d point;
        pointCoords.GetTuple(pointId, point.GetData());
        auto && point2d = reinterpret_cast<vtkVector2d &>(point);

        const double distance = distanceToLine(point2d);

        if (distance > maxAllowedDistance)
        {
            continue;
        }

        const double t = positionOnLine(point2d);
        static const double e = 5.e-7;
        if (t < 0.0 - e || t > 1.0 + e)
        {
            continue;
        }

        selectedPoints->InsertNextValue(pointId);
        if (generateGeometry)
        {
            extractedPoints->InsertNextPoint(point.GetData());
        }

        if (computePositionOnLine)
        {
            positionsOnLine->InsertNextValue(t);
        }

        if (this->PassDistanceToLine)
        {
            distancesToLine->InsertNextValue(distance);
        }
    }

    const vtkIdType outputNumPoints = selectedPoints->GetNumberOfTuples();

    if (generateSelection)
    {
        // Port 0: create selection information
        auto selectionNode = vtkSmartPointer<vtkSelectionNode>::New();
        selectionNode->SetFieldType(vtkSelectionNode::POINT);
        selectionNode->SetContentType(vtkSelectionNode::INDICES);
        selectionNode->SetSelectionList(selectedPoints);
        selectionOutput->AddNode(selectionNode);
    }


    // Port 1: create extracted point output
    if (!generateGeometry)
    {
        return 1;
    }

    // Sort output point w.r.t to the orientation of the line segment.

    std::vector<vtkIdType> sortIndices;

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
        sortIndices.resize(static_cast<size_t>(outputNumPoints));
        std::iota(sortIndices.begin(), sortIndices.end(), 0);
        vtkSortDataArray::GenerateSortIndices(
            positionsOnLine->GetDataType(),
            positionsOnLine->GetVoidPointer(0),
            outputNumPoints,
            positionsOnLine->GetNumberOfComponents(),
            0,
            sortIndices.data());

        vtkSortDataArray::ShuffleArray(
            sortIndices.data(),
            extractedPoints->GetDataType(),
            outputNumPoints,
            extractedPoints->GetData()->GetNumberOfComponents(),
            extractedPoints->GetData(),
            extractedPoints->GetData()->GetVoidPointer(0),
            0);
    }

    // pass input point data to output points (centroids)
    extractionPointData->CopyAllocate(inputPointData, outputNumPoints);

    std::function<vtkIdType(vtkIdType)> idGetter;
    if (this->Sorting == SortMode::SortPoints)
    {
        // need to reorder all attributes
        idGetter = [&sortIndices] (vtkIdType i) -> vtkIdType {
            return sortIndices[i];
        };
    }
    else
    {
        // keep the ordering
        idGetter = [] (vtkIdType i) -> vtkIdType { return i; };
    }

    for (vtkIdType outputPointId = 0; outputPointId < outputNumPoints; ++outputPointId)
    {
        const vtkIdType beforeReorderPointId = idGetter(outputPointId);
        const vtkIdType inputPointId = selectedPoints->GetValue(beforeReorderPointId);

        extractionPointData->CopyData(
            inputPointData, inputPointId,
            outputPointId);
    }

    auto verts = vtkSmartPointer<vtkCellArray>::New();
    verts->InsertNextCell(extractedPointIds.size(), extractedPointIds.data());

    extractedOutput->SetPoints(extractedPoints);
    extractedOutput->SetVerts(verts);

    if (this->PassPositionOnLine)
    {
        extractionPointData->AddArray(positionsOnLine);
    }

    if (this->PassDistanceToLine)
    {
        if (this->Sorting == SortMode::SortPoints)
        {
            vtkSortDataArray::ShuffleArray(
                sortIndices.data(),
                distancesToLine->GetDataType(),
                outputNumPoints,
                distancesToLine->GetNumberOfComponents(),
                distancesToLine,
                distancesToLine->GetVoidPointer(0),
                0);
        }
        extractionPointData->AddArray(distancesToLine);
    }

    return 1;
}
