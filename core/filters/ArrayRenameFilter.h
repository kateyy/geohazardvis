#pragma once

#include <vtkDataSetAlgorithm.h>

#include <core/core_api.h>


/** Renames current point data scalar array. */
class CORE_API ArrayRenameFilter : public vtkDataSetAlgorithm
{
public:
    static ArrayRenameFilter * New();
    vtkTypeMacro(ArrayRenameFilter, vtkDataSetAlgorithm);

    vtkGetStringMacro(ScalarsName);
    vtkSetStringMacro(ScalarsName);


protected:
    ArrayRenameFilter();

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    ArrayRenameFilter(const ArrayRenameFilter&) = delete;
    void operator=(const ArrayRenameFilter&) = delete;

private:
    char * ScalarsName;
};
