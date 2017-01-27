#pragma once

#include <vtkPassInputTypeAlgorithm.h>

#include <core/core_api.h>


/** Requests one specific time step from the upstream, if it is available.
 * It won't request any temporal data at all if the selected time step is not reported to be
 * available in the upstream. */
class CORE_API ExtractTimeStep : public vtkPassInputTypeAlgorithm
{
public:
    static ExtractTimeStep * New();
    vtkTypeMacro(ExtractTimeStep, vtkPassInputTypeAlgorithm);

    vtkGetMacro(TimeStep, double);
    vtkSetMacro(TimeStep, double);

protected:
    ExtractTimeStep();
    ~ExtractTimeStep() override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    double TimeStep;

private:
    ExtractTimeStep(const ExtractTimeStep &) = delete;
    void operator=(const ExtractTimeStep &) = delete;
};
