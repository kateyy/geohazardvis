#pragma once

#include <vtkDataSetAlgorithm.h>

#include <core/core_api.h>


/**
 * Compute the difference between scalars at two time steps provided by an upstream algorithm.
 *
 * The scalar difference is computed from the first to the second time step, thus: d = t1 - t0
 */
class CORE_API TemporalDifferenceFilter : public vtkDataSetAlgorithm
{
public:
    vtkTypeMacro(TemporalDifferenceFilter, vtkDataSetAlgorithm);
    static TemporalDifferenceFilter * New();

    void PrintSelf(std::ostream & os, vtkIndent indent) override;

    vtkGetMacro(TimeStep0, double);
    vtkSetMacro(TimeStep0, double);

    vtkGetMacro(TimeStep1, double);
    vtkSetMacro(TimeStep1, double);

protected:
    TemporalDifferenceFilter();
    ~TemporalDifferenceFilter() override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestUpdateExtent(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    bool InitProcess(vtkDataSet & input, vtkDataSet & output);
    bool ComputeDifferences(vtkDataSet & input, vtkDataSet & output);

private:
    double TimeStep0;
    double TimeStep1;

    // Used when iterating the pipeline to keep track of which time step we are on.
    enum class ProcessStep
    {
        init, difference, done
    };
    ProcessStep CurrentProcessStep;

private:
    TemporalDifferenceFilter(const TemporalDifferenceFilter &) = delete;
    void operator=(const TemporalDifferenceFilter &) = delete;
};
