#pragma once

#include <vtkPolyDataAlgorithm.h>

#include <core/core_api.h>


class CORE_API CentroidAsScalarsFilter : public vtkPolyDataAlgorithm
{
public:
    vtkTypeMacro(CentroidAsScalarsFilter, vtkPolyDataAlgorithm);

    static CentroidAsScalarsFilter * New();

protected:
    CentroidAsScalarsFilter();
    ~CentroidAsScalarsFilter() override;

    int RequestData(
        vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    CentroidAsScalarsFilter(const CentroidAsScalarsFilter&) = delete;
    void operator=(const CentroidAsScalarsFilter&) = delete;
};
