#include "CentroidAsScalarsFilter.h"

#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkCellData.h>


vtkStandardNewMacro(CentroidAsScalarsFilter);


CentroidAsScalarsFilter::CentroidAsScalarsFilter()
    : Superclass()
{
    SetNumberOfInputPorts(2);
    SetNumberOfOutputPorts(1);
}

CentroidAsScalarsFilter::~CentroidAsScalarsFilter() = default;

int CentroidAsScalarsFilter::RequestData(vtkInformation * vtkNotUsed(request),
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto input = vtkPolyData::GetData(inputVector[0], 0);
    auto centroidsPoly = vtkPolyData::GetData(inputVector[1], 0);
    auto centroids = centroidsPoly->GetPoints()->GetData();
    auto output = vtkPolyData::GetData(outputVector, 0);

    // Check the size of the input.
    const vtkIdType numCells = input->GetNumberOfCells();
    if (numCells <= 0 || !centroids)
    {
        vtkDebugMacro("No input!");
        return 1;
    }
    if (numCells != centroids->GetNumberOfTuples())
    {
        vtkDebugMacro("Number of cells doesn't match number of centroid points!");
        return 1;
    }

    // Copy all the input geometry and data to the output.
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());

    // Add the new scalars array to the output.
    output->GetCellData()->SetScalars(centroids);

    return 1;
}
