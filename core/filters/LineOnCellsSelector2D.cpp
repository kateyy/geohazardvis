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

#include "LineOnCellsSelector2D.h"

#include <cmath>
#include <functional>
#include <numeric>
#include <vector>

#include <vtkCellData.h>
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


vtkStandardNewMacro(LineOnCellsSelector2D);


LineOnCellsSelector2D::LineOnCellsSelector2D()
    : Superclass()
    , Sorting{ SortMode::SortPoints }
    , PassPositionOnLine{ true }
    , PassDistanceToLine{ true }
{
    this->SetNumberOfInputPorts(2);
    this->SetNumberOfOutputPorts(2);
}

LineOnCellsSelector2D::~LineOnCellsSelector2D() = default;

vtkPolyData * LineOnCellsSelector2D::GetExtractedPoints()
{
    return static_cast<vtkPolyData *>(this->GetOutputDataObject(1));
}

int LineOnCellsSelector2D::FillInputPortInformation(int port, vtkInformation * info)
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

int LineOnCellsSelector2D::FillOutputPortInformation(int port, vtkInformation * info)
{
    if (port == 1)
    {
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");

        return 1;
    }

    return Superclass::FillOutputPortInformation(port, info);
}

int LineOnCellsSelector2D::RequestData(vtkInformation * /*request*/,
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
        vtkWarningMacro(<< "LineOnCellsSelector2D: No points in input.");
        return 1;
    }
    if (numInputCells == 0)
    {
        vtkWarningMacro(<< "LineOnCellsSelector2D: No cells in input.");
        return 1;
    }

    if (numInputCells != centersInput->GetNumberOfPoints())
    {
        vtkErrorMacro(<< "LineOnCellsSelector2D: Number of input cell centers does not match number of input cells.");
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
    auto & cellPointCoords = *cellInput->GetPoints()->GetData();
    for (vtkIdType i = 0; i < numInputPoints; ++i)
    {
        signedPointDistances[i] = signedDistanceToLine({
            cellPointCoords.GetComponent(i, 0),
            cellPointCoords.GetComponent(i, 1) });
    }


    auto selectedCells = vtkSmartPointer<vtkIdTypeArray>::New();
    selectedCells->SetName("OriginalCellIds");

    vtkSmartPointer<vtkDoubleArray> positionsOnLine;
    const bool computePositionsOnLine = this->PassPositionOnLine || this->Sorting != SortNone;
    if (computePositionsOnLine)
    {
        positionsOnLine = vtkSmartPointer<vtkDoubleArray>::New();
        positionsOnLine->SetName("positionOnLine");
    }

    vtkSmartPointer<vtkDoubleArray> distancesToLine;
    if (this->PassDistanceToLine)
    {
        distancesToLine = vtkSmartPointer<vtkDoubleArray>::New();
        distancesToLine->SetName("DistanceToLine");
    }


    auto extractedPoints = vtkSmartPointer<vtkPoints>::New();
    std::vector<vtkIdType> extractedPointIds;

    auto pointIdList = vtkSmartPointer<vtkIdList>::New();
    auto & centersPointCoords = *centersInput->GetPoints()->GetData();
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

        vtkVector3d centroid;
        centersPointCoords.GetTuple(cellId, centroid.GetData());
        const vtkVector2d centroid2d = { centroid[0], centroid[1] };

        const double t = positionOnLine(centroid2d);

        if (t < 0 || t > 1)
        {
            continue;
        }

        selectedCells->InsertNextValue(cellId);

        extractedPointIds.push_back(
            extractedPoints->InsertNextPoint(centroid.GetData()));

        if (computePositionsOnLine)
        {
            positionsOnLine->InsertNextValue(t);
        }

        if (this->PassDistanceToLine)
        {
            distancesToLine->InsertNextValue(std::abs(signedDistanceToLine(centroid2d)));
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

    // pass input cell data to output points (centroids)
    extractionPointData->CopyAllocate(inputCellData, outputNumPoints);

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
        idGetter = [] (vtkIdType i) -> vtkIdType {
            return i;
        };
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
