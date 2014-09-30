#pragma once

#include <vtkPolyDataAlgorithm.h>

#include <core/core_api.h>


class CORE_API CentroidAsScalarsFilter : public vtkPolyDataAlgorithm
{
public:
    vtkTypeMacro(CentroidAsScalarsFilter, vtkPolyDataAlgorithm);

    static CentroidAsScalarsFilter * New();

    vtkSetClampMacro(Component, unsigned, 0, 2);
    vtkGetMacro(Component, unsigned);

protected:
    CentroidAsScalarsFilter();
    ~CentroidAsScalarsFilter();

    int RequestData(
        vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    CentroidAsScalarsFilter(const CentroidAsScalarsFilter&);    // Not implemented.
    void operator=(const CentroidAsScalarsFilter&);             // Not implemented.

    unsigned Component;
};
