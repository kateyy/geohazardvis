#include <gtest/gtest.h>

#include <algorithm>
#include <cassert>

#include <vtkExecutive.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTemporalInterpolator.h>
#include <vtkTemporalSnapToTimeStep.h>
#include <vtkVector.h>

#include <core/filters/TemporalDataSource.h>
#include <core/utility/vtkVector_print.h>


class TemporalDataSource_test : public ::testing::Test
{
public:
    static const std::vector<double> & timeSteps()
    {
        static const std::vector<double> ts = { 42.1, 42.2, 42.3, 42.4 };
        return ts;
    }

    static const char * attributeName()
    {
        static const char * const name = "temporalAttr";
        return name;
    }

    static const std::vector<float> & timeStepValues()
    {
        static const std::vector<float> ts = { 1.0f, 2.0f, 3.0f, 4.0f };
        return ts;
    }
    static vtkSmartPointer<TemporalDataSource> createSource(
        const std::vector<double> & steps = timeSteps(),
        const std::vector<float> & values = timeStepValues(),
        const char * attrName = attributeName())
    {
        assert(steps.size() == values.size());
        auto dataSet = vtkSmartPointer<vtkImageData>::New();
        auto temporalSource = vtkSmartPointer<TemporalDataSource>::New();
        temporalSource->SetInputDataObject(dataSet);
        const auto id = temporalSource->AddTemporalAttribute(
            TemporalDataSource::AttributeLocation::POINT_DATA, attrName);

        for (size_t i = 0; i < steps.size(); ++i)
        {
            auto data = vtkSmartPointer<vtkFloatArray>::New();
            data->SetNumberOfValues(2);
            data->SetValue(0, values[i]);
            data->SetValue(1, values[i]);

            temporalSource->SetTemporalAttributeTimeStep(
                TemporalDataSource::AttributeLocation::POINT_DATA, id,
                steps[i],
                data);
        }
        return temporalSource;
    }
};


TEST_F(TemporalDataSource_test, TemporalInformation)
{
    auto source = createSource();
    ASSERT_TRUE(source->GetExecutive()->UpdateInformation());
    auto outInfo = source->GetOutputInformation(0);
    // vtkStreamingDemandDrivenPipeline temporal pipeline requires both keys to be set
    ASSERT_TRUE(outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()));
    ASSERT_TRUE(outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()));

    const auto numTimeSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    const auto rangeSize = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    ASSERT_EQ(timeSteps().size(), static_cast<size_t>(numTimeSteps));
    ASSERT_EQ(2, rangeSize);

    const auto timeStepsPtr = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    const auto outTimeSteps = std::vector<double>(timeStepsPtr, timeStepsPtr + numTimeSteps);
    ASSERT_EQ(timeSteps(), outTimeSteps);

    const auto expectedRange = vtkVector2d(timeSteps().front(), timeSteps().back());
    const auto outRangePtr = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    const auto outRange = vtkVector2d(outRangePtr[0], outRangePtr[1]);
    ASSERT_EQ(expectedRange, outRange);
}

TEST_F(TemporalDataSource_test, TemporalExtent)
{
    auto source = createSource();
    ASSERT_TRUE(source->GetExecutive()->UpdateInformation());
    auto outInfo = source->GetOutputInformation(0);
    
    for (auto && timeStep : timeSteps())
    {
        outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeStep);
        source->PropagateUpdateExtent();
        ASSERT_TRUE(outInfo->Has(vtkDataObject::DATA_TIME_STEP()));
        ASSERT_EQ(timeStep, outInfo->Get(vtkDataObject::DATA_TIME_STEP()));
    }
}

TEST_F(TemporalDataSource_test, TemporalData)
{
    auto source = createSource();
    ASSERT_TRUE(source->GetExecutive()->UpdateInformation());
    auto outInfo = source->GetOutputInformation(0);

    for (size_t i = 0; i < timeSteps().size(); ++i)
    {
        const auto & timeStep = timeSteps()[i];
        outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeStep);
        ASSERT_TRUE(source->GetExecutive()->Update());
        ASSERT_EQ(1, source->GetOutput()->GetPointData()->GetNumberOfArrays());
        // TemporalDataSource should set the array name to the name of the attribute
        auto array = vtkArrayDownCast<vtkFloatArray>(
            source->GetOutput()->GetPointData()->GetAbstractArray(attributeName()));
        ASSERT_TRUE(array);
        ASSERT_EQ(2, array->GetNumberOfValues());
        ASSERT_EQ(timeStepValues()[i], array->GetComponent(0, 0));
        ASSERT_EQ(timeStepValues()[i], array->GetComponent(1, 0));
    }
}

TEST_F(TemporalDataSource_test, SortTimeSteps)
{
    auto steps = timeSteps();
    auto values = timeStepValues();
    std::prev_permutation(steps.begin(), steps.end());
    std::prev_permutation(steps.begin(), steps.end());
    std::prev_permutation(values.begin(), values.end());
    std::prev_permutation(values.begin(), values.end());

    auto source = createSource(steps, values);

    ASSERT_TRUE(source->GetExecutive()->UpdateInformation());
    auto outInfo = source->GetOutputInformation(0);

    const auto numTimeSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    const auto timeStepsPtr = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    const auto outTimeSteps = std::vector<double>(timeStepsPtr, timeStepsPtr + numTimeSteps);
    // time steps should be in order again
    ASSERT_EQ(timeSteps(), outTimeSteps);

    const auto expectedRange = vtkVector2d(timeSteps().front(), timeSteps().back());
    const auto outRangePtr = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    const auto outRange = vtkVector2d(outRangePtr[0], outRangePtr[1]);
    ASSERT_EQ(expectedRange, outRange);

    for (size_t i = 0; i < steps.size(); ++i)
    {
        const auto timeStep = steps[i];
        outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeStep);
        ASSERT_TRUE(source->GetExecutive()->Update());
        ASSERT_EQ(1, source->GetOutput()->GetPointData()->GetNumberOfArrays());
        auto array = vtkArrayDownCast<vtkFloatArray>(
            source->GetOutput()->GetPointData()->GetAbstractArray(attributeName()));
        ASSERT_TRUE(array);
        ASSERT_EQ(2, array->GetNumberOfValues());
        ASSERT_EQ(values[i], array->GetComponent(0, 0));
        ASSERT_EQ(values[i], array->GetComponent(1, 0));
    }
}

TEST_F(TemporalDataSource_test, Use_vtkTemporalSnapToTimeStep)
{
    // Make sure that the filter correctly works with VTK's implementations

    const auto timeStep = 0.2 * timeSteps()[0] + 0.8 * timeSteps()[1];
    const auto expectedValue = timeStepValues()[1];

    auto source = createSource();

    auto snap = vtkSmartPointer<vtkTemporalSnapToTimeStep>::New();
    snap->SetInputConnection(source->GetOutputPort());

    ASSERT_TRUE(snap->GetExecutive()->UpdateInformation());
    auto & outInfo = *snap->GetOutputInformation(0);
    outInfo.Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeStep);

    ASSERT_TRUE(snap->GetExecutive()->Update());

    // VTK patch (> 7.1): inherit vtkTemporalSnapToTimeStep from vtkPassInputTypeAlgorithm
    snap->GetOutputDataObject(0)->IsTypeOf(source->GetInputDataObject(0, 0)->GetClassName());
    auto outData = vtkDataSet::SafeDownCast(snap->GetOutputDataObject(0));
    ASSERT_TRUE(outData);
    auto array = vtkArrayDownCast<vtkFloatArray>(
        outData->GetPointData()->GetAbstractArray(attributeName()));
    ASSERT_TRUE(array);
    ASSERT_EQ(2, array->GetNumberOfValues());

    ASSERT_EQ(expectedValue, array->GetValue(0));
    ASSERT_EQ(expectedValue, array->GetValue(1));
}

TEST_F(TemporalDataSource_test, InterpolateTimeSteps)
{
    // Make sure that the filter correctly works with VTK's implementations

    const auto timeStep = 0.2 * timeSteps()[0] + 0.8 * timeSteps()[1];
    const auto expectedValue = 0.2f * timeStepValues()[0] + 0.8f * timeStepValues()[1];

    auto source = createSource();

    auto snap = vtkSmartPointer<vtkTemporalInterpolator>::New();
    snap->SetInputConnection(source->GetOutputPort());

    ASSERT_TRUE(snap->GetExecutive()->UpdateInformation());
    auto & outInfo = *snap->GetOutputInformation(0);
    outInfo.Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeStep);

    ASSERT_TRUE(snap->GetExecutive()->Update());

    auto outData = vtkDataSet::SafeDownCast(snap->GetOutputDataObject(0));
    ASSERT_TRUE(outData);
    auto array = vtkArrayDownCast<vtkFloatArray>(
        outData->GetPointData()->GetAbstractArray(attributeName()));
    ASSERT_TRUE(array);
    ASSERT_EQ(2, array->GetNumberOfValues());

    ASSERT_FLOAT_EQ(expectedValue, array->GetValue(0));
    ASSERT_FLOAT_EQ(expectedValue, array->GetValue(1));
}
