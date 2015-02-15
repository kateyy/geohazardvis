#include <core/filters/DataSetCleaner.h>

#include <algorithm>

#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>


vtkStandardNewMacro(DataSetCleaner);


DataSetCleaner::DataSetCleaner()
    : vtkDataSetAlgorithm()
{
}

DataSetCleaner::~DataSetCleaner()
{
}

int DataSetCleaner::RequestData(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{
    vtkDataSet * input = vtkDataSet::GetData(inputVector[0]);
    vtkDataSet * output = vtkDataSet::GetData(outputVector);

    if (!input || !output)
        return 1;

    // Copy all input attributes first
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    output->GetFieldData()->PassData(input->GetFieldData());

    // Remove temp/helper etc arrays
    for (const std::string & name : this->ArraysToRemove)
    {
        output->GetPointData()->RemoveArray(name.c_str());
        output->GetCellData()->RemoveArray(name.c_str());
        output->GetFieldData()->RemoveArray(name.c_str());
    }

    return 1;
}

void DataSetCleaner::AddArrayToRemove(const char * c_name)
{
    std::string name{ c_name };

    auto it = std::find(
        this->ArraysToRemove.begin(),
        this->ArraysToRemove.end(),
        name);

    if (it != this->ArraysToRemove.end())
        return; // already in list

    this->ArraysToRemove.push_back(name);

    this->Modified();
}

void DataSetCleaner::RemoveArrayToRemove(const char * c_name)
{
    std::string name{ c_name };

    auto it = std::find(
        this->ArraysToRemove.begin(),
        this->ArraysToRemove.end(),
        name);

    if (it == this->ArraysToRemove.end())
        return; // not in list

    this->ArraysToRemove.erase(it);

    this->Modified();
}

void DataSetCleaner::ClearArraysToRemove()
{
    if (this->ArraysToRemove.empty())
        return;

    this->ArraysToRemove.clear();

    this->Modified();
}

void DataSetCleaner::PrintSelf(ostream & os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);
    os << indent << "ArraysToRemove:" << std::endl;
    for (const std::string & name : this->ArraysToRemove)
        os << "\t" << name << std::endl;
}
