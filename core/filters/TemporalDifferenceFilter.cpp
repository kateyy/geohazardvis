#include "TemporalDifferenceFilter.h"

#include <vtkArrayDispatch.h>
#include <vtkAssume.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataArrayAccessor.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkSMPTools.h>
#include <vtkStreamingDemandDrivenPipeline.h>


vtkStandardNewMacro(TemporalDifferenceFilter);

namespace
{

struct DifferenceWorker
{
    vtkSmartPointer<vtkDataArray> output;

    template<typename Array_t>
    void operator()(Array_t * inT0Array, Array_t * inT1Array)
    {
        const int numComponents = inT0Array->GetNumberOfComponents();
        const vtkIdType numTuples = inT0Array->GetNumberOfTuples();
        VTK_ASSUME(numComponents == inT1Array->GetNumberOfComponents());
        VTK_ASSUME(numTuples == inT1Array->GetNumberOfTuples());

        output.TakeReference(inT0Array->NewInstance());
        auto outArray = static_cast<Array_t *>(output.Get());
        outArray->SetNumberOfComponents(numComponents);
        outArray->SetNumberOfTuples(numTuples);
        outArray->SetName(inT0Array->GetName());
        outArray->GetInformation()->Set(vtkDataArray::UNITS_LABEL(),
            inT0Array->GetInformation()->Get(vtkDataArray::UNITS_LABEL()));

        vtkDataArrayAccessor<Array_t> inT0(inT0Array);
        vtkDataArrayAccessor<Array_t> inT1(inT1Array);
        vtkDataArrayAccessor<Array_t> out(outArray);

        vtkSMPTools::For(0, inT0Array->GetNumberOfTuples(),
            [inT0, inT1, out, numComponents] (vtkIdType begin, vtkIdType end)
        {
            for (vtkIdType i = begin; i < end; ++i)
            {
                for (vtkIdType c = 0; c < numComponents; ++c)
                {
                    out.Set(i, c, inT1.Get(i, c) - inT0.Get(i, c));
                }
            }
        });
    }
};

}


TemporalDifferenceFilter::TemporalDifferenceFilter()
    : Superclass()
    , TimeStep0{ 0 }
    , TimeStep1{ 1 }
    , CurrentProcessStep{ ProcessStep::init }
{
}

TemporalDifferenceFilter::~TemporalDifferenceFilter() = default;

void TemporalDifferenceFilter::PrintSelf(std::ostream & os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);

    os << indent << "TimeStep0: " << this->TimeStep0 << endl;
    os << indent << "TimeStep1: " << this->TimeStep1 << endl;
}

int TemporalDifferenceFilter::RequestInformation(vtkInformation * request,
    vtkInformationVector ** inputVector, vtkInformationVector * outputVector)
{
    if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
    {
        return 0;
    }

    auto outInfo = outputVector->GetInformationObject(0);

    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

    return 1;
}

int TemporalDifferenceFilter::RequestUpdateExtent(vtkInformation * request,
    vtkInformationVector ** inputVector, vtkInformationVector * outputVector)
{
    auto inInfo = inputVector[0]->GetInformationObject(0);

    // The RequestData method will tell the pipeline executive to iterate the upstream pipeline to
    // get the required time steps in order. The executive in turn will call this method to get the
    // extent request for each iteration (in this case the time step).

    const auto requestTime = this->CurrentProcessStep == ProcessStep::init
        ? this->TimeStep0
        : this->TimeStep1;

    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), requestTime);

    return Superclass::RequestUpdateExtent(request, inputVector, outputVector);
}

int TemporalDifferenceFilter::RequestData(vtkInformation * request,
    vtkInformationVector ** inputVector, vtkInformationVector * outputVector)
{
    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);

    auto input = vtkDataSet::GetData(inInfo);
    auto output = vtkDataSet::GetData(outInfo);

    if (this->CurrentProcessStep == ProcessStep::init)
    {
        const bool result = this->InitProcess(*input, *output);

        // continue with the execution until both time steps are read
        request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);

        this->CurrentProcessStep = ProcessStep::difference;

        return result;
    }

    if (this->CurrentProcessStep == ProcessStep::difference)
    {
        const bool result = this->ComputeDifferences(*input, *output);

        this->CurrentProcessStep = ProcessStep::done;

        return result;
    }

    if (this->CurrentProcessStep == ProcessStep::done)
    {
        // Done. No further execution required until inputs/parameters change.
        request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
        this->CurrentProcessStep = ProcessStep::init;
        return 1;
    }

    // This should not happen...
    return 0;
}

bool TemporalDifferenceFilter::InitProcess(vtkDataSet & input, vtkDataSet & output)
{
    this->CurrentProcessStep = ProcessStep::difference;
    // Just pass all input data to the output, check in the next step where to compute
    // differences.
    output.CopyStructure(&input);
    output.GetPointData()->PassData(input.GetPointData());
    output.GetCellData()->PassData(output.GetCellData());

    return true;
}

bool TemporalDifferenceFilter::ComputeDifferences(vtkDataSet & input, vtkDataSet & output)
{
    // Fetch the second time step, check if it's required to compute a difference.
    auto TemporalDifferenceFilter =
        [this] (vtkFieldData & inFields, vtkFieldData & outFields) -> bool
    {
        const auto numArrays = inFields.GetNumberOfArrays();
        if (outFields.GetNumberOfArrays() != numArrays)
        {
            vtkWarningMacro(<< "Unexpected attribute mismatch in input/output.");
            return false;
        }

        // Find arrays that need to be copied. Never overwrite input data arrays!
        for (int i = 0; i < numArrays; ++i)
        {
            auto inData = inFields.GetArray(i);
            if (!inData || !inData->GetName())
            {
                continue;   // numerical named arrays are required
            }

            // Replacing arrays by a copy of them might change array indexes.
            // Thus, identify output arrays by name.
            int outIndex = -1;
            auto outData = outFields.GetArray(inData->GetName(), outIndex);

            const bool isTemporal = inData->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()) != 0;
            if (!isTemporal)
            {
                continue;
            }

            vtkSmartPointer<vtkDataArray> resultArray;

            if (inData == outData)
            {
                // Same array at different time steps -> expected difference is zero, so just
                // take a shortcut here.
                auto zeros = vtkSmartPointer<vtkDataArray>::Take(inData->NewInstance());
                zeros->SetNumberOfComponents(inData->GetNumberOfComponents());
                zeros->SetNumberOfTuples(inData->GetNumberOfTuples());
                zeros->Fill(0.0);
                zeros->SetName(inData->GetName());
                resultArray = zeros;
            }
            else if(!outData || inData->GetNumberOfValues() != outData->GetNumberOfValues())
            {
                vtkWarningMacro(<< "Unexpected attribute mismatch in input/output.");
                return false;
            }
            else
            {
                // Compute the difference for the current attribute
                using Dispatcher = vtkArrayDispatch::Dispatch2BySameValueType<
                    vtkArrayDispatch::AllTypes>;
                DifferenceWorker worker;
                // outData still contains t0 from the initialize loop.
                // inData already contains the recently requested t1.
                if (!Dispatcher::Execute(outData, inData, worker))
                {
                    worker(outData, inData);
                }
                resultArray = worker.output;
            }

            // Replace the array in the output by the difference between both time steps.
            outFields.RemoveArray(outIndex);
            outFields.AddArray(resultArray);

        }
        return true;
    };

    if (!TemporalDifferenceFilter(*input.GetPointData(), *output.GetPointData()))
    {
        return false;
    }
    if (!TemporalDifferenceFilter(*input.GetCellData(), *output.GetCellData()))
    {
        return false;
    }
    if (!TemporalDifferenceFilter(*input.GetFieldData(), *output.GetFieldData()))
    {
        return false;
    }

    return true;
}
