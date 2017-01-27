#include "TemporalDataSource.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <limits>

#include <vtkAbstractArray.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkStreamingDemandDrivenPipeline.h>


vtkStandardNewMacro(TemporalDataSource);

TemporalDataSource::TemporalDataSource()
    : Superclass()
{
}

TemporalDataSource::~TemporalDataSource() = default;

int TemporalDataSource::AddTemporalAttribute(AttributeLocation attributeLoc, const vtkStdString & name)
{
    auto & vectorForAttributeType = temporalData(attributeLoc);

    auto index = TemporalAttributeIndex(attributeLoc, name);
    if (index >= 0)
    {
        // The name is already in use
        return index;
    }

    if (vectorForAttributeType.size() >= static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        return -1;
    }

    vectorForAttributeType.push_back({ name, {} });
    return static_cast<int>(vectorForAttributeType.size() - 1);
}

int TemporalDataSource::TemporalAttributeIndex(AttributeLocation attributeLoc, const vtkStdString & name)
{
    auto & vectorForAttributeType = temporalData(attributeLoc);

    auto it = std::find_if(vectorForAttributeType.begin(), vectorForAttributeType.end(),
        [&name] (const TemporalAttribute & attribute)
    {
        return attribute.Name == name;
    });

    if (it != vectorForAttributeType.end())
    {
        return static_cast<int>(it - vectorForAttributeType.begin());
    }

    return -1;
}

bool TemporalDataSource::RemoveTemporalAttribute(AttributeLocation attributeLoc, const vtkStdString & name)
{
    const auto & vectorForAttributeType = temporalData(attributeLoc);
    const auto it = std::find_if(vectorForAttributeType.begin(), vectorForAttributeType.end(),
        [&name] (const TemporalAttribute & attribute)
    {
        return attribute.Name == name;
    });
    if (it == vectorForAttributeType.end())
    {
        return false;
    }

    const auto index = std::distance(it, vectorForAttributeType.begin());

    return RemoveTemporalAttribute(attributeLoc, static_cast<int>(index));
}

bool TemporalDataSource::RemoveTemporalAttribute(AttributeLocation attributeLoc, int temporalAttributeIndex)
{
    auto & vectorForAttributeType = temporalData(attributeLoc);

    const auto idx = static_cast<size_t>(temporalAttributeIndex);

    if (temporalAttributeIndex < 0 || idx >= vectorForAttributeType.size())
    {
        return false;
    }

    vectorForAttributeType.erase(std::next(vectorForAttributeType.begin(), idx));

    return true;
}

bool TemporalDataSource::SetTemporalAttributeTimeStep(
    const AttributeLocation attributeLoc,
    const int temporalAttributeIndex,
    vtkAbstractArray * array)
{
    if (!array || !array->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
    {
        return false;
    }

    return SetTemporalAttributeTimeStep(
        attributeLoc,
        temporalAttributeIndex,
        array->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP()),
        array);
}

bool TemporalDataSource::SetTemporalAttributeTimeStep(
    const AttributeLocation attributeLoc,
    const int temporalAttributeIndex,
    const double timeStep,
    vtkAbstractArray * array)
{
    auto & vectorForAttributeType = temporalData(attributeLoc);

    const auto idx = static_cast<size_t>(temporalAttributeIndex);

    if (temporalAttributeIndex < 0 || idx >= vectorForAttributeType.size())
    {
        return false;
    }

    auto & attribute = vectorForAttributeType[idx];

    if (array)
    {
        array->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), timeStep);
        array->SetName(attribute.Name);
    }

    // If passed in order, just append to the end
    if (attribute.Data.empty() || attribute.Data.back().TimeStep < timeStep)
    {
        attribute.Data.push_back({ timeStep, array });
        return true;
    }

    auto it = std::lower_bound(attribute.Data.begin(), attribute.Data.end(), timeStep);
    if (it == attribute.Data.end())
    {
        // no smaller value, insert as first element
        it = attribute.Data.begin();
    }
    else if (it->TimeStep == timeStep)
    {
        // replace data with same time stamp
        it->Attribute = array;
        return true;
    }

    attribute.Data.insert(it, { timeStep, array });

    return true;
}

int TemporalDataSource::ProcessRequest(
    vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_TIME())
        || request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
        return this->RequestUpdateExtent(request, inputVector, outputVector);
    }

    return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int TemporalDataSource::RequestInformation(
    vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    if (!Superclass::RequestInformation(request, inputVector, outputVector))
    {
        return 0;
    }

    std::vector<double> timeSteps;

    // Gather time steps from all temporal attributes in supported attribute locations.
    // Make sure that this list is sorted and unified
    for (const auto & attrType : this->TemporalData)
    {
        for (const auto & attribute : attrType)
        {
            assert(std::is_sorted(attribute.Data.begin(), attribute.Data.end()));

            // append sorted time steps for the current attribute to previously sorted time steps
            const size_t previousNumTimeSteps = timeSteps.size();
            for (const auto & timeStep : attribute.Data)
            {
                timeSteps.push_back(timeStep.TimeStep);
            }

            std::inplace_merge(timeSteps.begin(), timeSteps.begin() + previousNumTimeSteps, timeSteps.end());
        }
    }

    assert(std::is_sorted(timeSteps.begin(), timeSteps.end()));
    const auto endIt = std::unique(timeSteps.begin(), timeSteps.end());
    timeSteps.erase(endIt, timeSteps.end());

    if (timeSteps.size() > static_cast<unsigned>(std::numeric_limits<int>::max()))
    {
        vtkErrorMacro(<< "Number of time steps exceeds range of int (" << timeSteps.size() << ")");
        return 0;
    }

    auto & outInfo = *outputVector->GetInformationObject(0);

    if (timeSteps.empty())
    {
    // no temporal information
    outInfo.Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo.Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    return 1;
    }

    outInfo.Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeSteps.data(),
        static_cast<int>(timeSteps.size()));

    const auto range = { timeSteps.front(), timeSteps.back() };
    outInfo.Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range.begin(), 2);

    return 1;
}

int TemporalDataSource::RequestUpdateExtent(
    vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    if (!Superclass::RequestUpdateExtent(request, inputVector, outputVector))
    {
        return 0;
    }

    auto outInfo = outputVector->GetInformationObject(0);
    if (!outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
        // Information update step didn't add any time steps.
        return 1;
    }
    const int numTimeSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (numTimeSteps == 0)
    {
        return 1;
    }

    if (!outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
        // no time dependent information requested
        return 1;
    }

    const double requestedTimeStep =
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    const double * timeSteps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    const auto endIt = timeSteps + numTimeSteps;
    const auto it = std::lower_bound(timeSteps, endIt, requestedTimeStep);

    if (it == endIt && *it != requestedTimeStep)
    {
        // Downstream should use, e.g, vtkTemporalSnapToTimeStep in such cases.
        vtkWarningMacro(<< "Requested time step not available: " << requestedTimeStep);
        outInfo->Remove(vtkDataObject::DATA_TIME_STEP());
        return 1;
    }

    outInfo->Set(vtkDataObject::DATA_TIME_STEP(), requestedTimeStep);

    return 1;
}

int TemporalDataSource::RequestData(
    vtkInformation * /*request*/,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);

    auto inData = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    auto outData = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    outData->CopyStructure(inData);
    outData->GetPointData()->PassData(inData->GetPointData());
    outData->GetCellData()->PassData(inData->GetCellData());

    if (!outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP())
        || !outInfo->Has(vtkDataObject::DATA_TIME_STEP()))
    {
        // No time dependent information requested or not available.
        return 1;
    }

    const double timeStep = outInfo->Get(vtkDataObject::DATA_TIME_STEP());

    auto appendArrays = [this, timeStep] (AttributeLocation location, vtkDataSetAttributes & dsa)
    {
        for (auto & attribute : temporalData(location))
        {
            const auto it = std::lower_bound(attribute.Data.begin(), attribute.Data.end(), timeStep);
            if (it == attribute.Data.end() || it->TimeStep != timeStep)
            {
                vtkWarningMacro(<< "Requested time step not available in attribute: "
                    << attribute.Name
                    << ", time step: " << timeStep);
                continue;
            }

            dsa.AddArray(it->Attribute);
        }
    };

    appendArrays(AttributeLocation::POINT_DATA, *outData->GetPointData());
    appendArrays(AttributeLocation::CELL_DATA, *outData->GetCellData());

    outData->Modified();

    return 1;
}

auto TemporalDataSource::temporalData(AttributeLocation attributeLoc) -> decltype(TemporalData)::value_type &
{
    return this->TemporalData[static_cast<size_t>(attributeLoc)];
}

bool TemporalDataSource::AttributeAtTimeStep::operator<(const AttributeAtTimeStep & other) const
{
    return this->TimeStep < other.TimeStep;
}

bool TemporalDataSource::AttributeAtTimeStep::operator<(double other) const
{
    return this->TimeStep < other;
}

bool operator<(double lhs, const TemporalDataSource::AttributeAtTimeStep & rhs)
{
    return lhs < rhs.TimeStep;
}
