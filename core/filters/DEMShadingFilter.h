#pragma once

#include <vtkImageAlgorithm.h>

#include <core/core_api.h>

class CORE_API DEMShadingFilter : public vtkImageAlgorithm
{
public:
    static DEMShadingFilter * New();
    vtkTypeMacro(DEMShadingFilter, vtkImageAlgorithm);

    vtkGetMacro(AmbientRatio, double);
    vtkSetClampMacro(AmbientRatio, double, 0.0, 1.0);

protected:
    DEMShadingFilter();
    ~DEMShadingFilter() override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    double AmbientRatio;

private:
    DEMShadingFilter(const DEMShadingFilter &) = delete;
    void operator=(const DEMShadingFilter &) = delete;
};
