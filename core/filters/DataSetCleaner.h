#pragma once

#include <core/core_api.h>
#include <string>
#include <vector>

#include <vtkDataSetAlgorithm.h>


// TODO missing update extent / information
// will not propagate upstream changes
class CORE_API DataSetCleaner : public vtkDataSetAlgorithm
{
public:
    static DataSetCleaner *New();
    vtkTypeMacro(DataSetCleaner, vtkDataSetAlgorithm);
    void PrintSelf(ostream& os, vtkIndent indent);

    void AddArrayToRemove(const char * name);
    void RemoveArrayToRemove(const char * name);
    void ClearArraysToRemove();

protected:
    DataSetCleaner();
    ~DataSetCleaner();

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    std::vector<std::string> ArraysToRemove;
};
