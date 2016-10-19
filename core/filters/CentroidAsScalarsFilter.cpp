#include "CentroidAsScalarsFilter.h"

#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkCellData.h>


vtkStandardNewMacro(CentroidAsScalarsFilter);


CentroidAsScalarsFilter::CentroidAsScalarsFilter()
    : Component(0)
{
    SetNumberOfInputPorts(2);
    SetNumberOfOutputPorts(1);
}

CentroidAsScalarsFilter::~CentroidAsScalarsFilter() = default;

int CentroidAsScalarsFilter::RequestData(vtkInformation *vtkNotUsed(request),
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{
    // get the input and output
    vtkPolyData * input = vtkPolyData::GetData(inputVector[0], 0);
    vtkPolyData * centroidsPoly = vtkPolyData::GetData(inputVector[1], 0);
    auto centroids = centroidsPoly->GetPoints()->GetData();
    vtkPolyData * output = vtkPolyData::GetData(outputVector, 0);

    // Check the size of the input.
    vtkIdType numCells = input->GetNumberOfCells();
    if (numCells < 1)
    {
        vtkDebugMacro("No input!");
        return 1;
    }
    if (numCells != centroids->GetNumberOfTuples())
    {
        vtkDebugMacro("Number of cells doesn't match number of centroid points!");
        return 1;
    }

    auto centroidScalars = vtkSmartPointer<vtkFloatArray>::New();
    centroidScalars->SetNumberOfComponents(1);
    centroidScalars->SetNumberOfTuples(numCells);

    for (vtkIdType i = 0; i < numCells; ++i)
    {
        centroidScalars->SetValue(i,
            static_cast<float>(centroids->GetComponent(i, this->Component)));
    }

    char componentName[2] = { static_cast<char>('x' + Component), '\0' };
    std::string name = std::string(componentName) + "-coordinate";
    centroidScalars->SetName(name.c_str());

    // Copy all the input geometry and data to the output.
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());

    // Add the new scalars array to the output.
    output->GetCellData()->AddArray(centroidScalars);

    return 1;
}
