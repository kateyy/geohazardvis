#pragma once

#include <vtkImageAlgorithm.h>

#include <core/core_api.h>


class CORE_API DEMApplyShadingToColors : public vtkImageAlgorithm
{
public:
    static DEMApplyShadingToColors * New();

    vtkTypeMacro(DEMApplyShadingToColors, vtkImageAlgorithm);

protected:
    DEMApplyShadingToColors();
    ~DEMApplyShadingToColors() override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    DEMApplyShadingToColors(const DEMApplyShadingToColors &) = delete;
    void operator=(const DEMApplyShadingToColors &) = delete;
};