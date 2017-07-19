/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <vtkDoubleArray.h>
#include <vtkExecutive.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <core/filters/TemporalDifferenceFilter.h>
#include <core/filters/TemporalDataSource.h>

class TemporalDifferenceFilter_test : public ::testing::Test
{
public:

    static std::vector<double> timeSteps()
    {
        return{ 0.0, 1.0, 2.0, 4.0, 17.0 };
    }

    static double getValue(int index, double timeStep)
    {
        return double(index) + double(index + 1.0) * timeStep * timeStep;
    }

    static vtkSmartPointer<TemporalDataSource> createSource(const std::vector<double> & timeSteps)
    {
        auto source = vtkSmartPointer<TemporalDataSource>::New();
        source->SetInputData(vtkNew<vtkPolyData>().Get());

        const auto idx0 = source->AddTemporalAttribute(
            TemporalDataSource::AttributeLocation::POINT_DATA, "temp");

        for (auto timeStep : timeSteps)
        {
            vtkNew<vtkDoubleArray> data;
            data->SetNumberOfValues(3);
            data->SetValue(0, getValue(0, timeStep));
            data->SetValue(1, getValue(1, timeStep));
            data->SetValue(2, getValue(2, timeStep));
            source->SetTemporalAttributeTimeStep(TemporalDataSource::AttributeLocation::POINT_DATA,
                idx0, timeStep, data.Get());
        }

        return source;
    }
};


TEST_F(TemporalDifferenceFilter_test, Difference)
{
    auto source = createSource(timeSteps());
    vtkNew<TemporalDifferenceFilter> filter;
    filter->SetInputConnection(source->GetOutputPort());

    filter->SetTimeStep0(1.0);
    filter->SetTimeStep1(2.0);
    ASSERT_TRUE(filter->GetExecutive()->Update());
    auto output = filter->GetOutput();
    ASSERT_TRUE(output);
    {
        auto outData = output->GetPointData()->GetArray("temp");
        ASSERT_TRUE(outData);
        auto outDataD = vtkDoubleArray::SafeDownCast(outData);
        ASSERT_TRUE(outDataD);

        ASSERT_DOUBLE_EQ(getValue(0, 2.0) - getValue(0, 1.0), outDataD->GetValue(0));
        ASSERT_DOUBLE_EQ(getValue(1, 2.0) - getValue(1, 1.0), outDataD->GetValue(1));
        ASSERT_DOUBLE_EQ(getValue(2, 2.0) - getValue(2, 1.0), outDataD->GetValue(2));
    }

    {
        filter->SetTimeStep0(2.0);
        filter->SetTimeStep1(17.0);
        ASSERT_TRUE(filter->GetExecutive()->Update());
        auto outDataD = vtkDoubleArray::SafeDownCast(output->GetPointData()->GetArray("temp"));
        ASSERT_TRUE(outDataD);

        ASSERT_DOUBLE_EQ(getValue(0, 17.0) - getValue(0, 2.0), outDataD->GetValue(0));
        ASSERT_DOUBLE_EQ(getValue(1, 17.0) - getValue(1, 2.0), outDataD->GetValue(1));
        ASSERT_DOUBLE_EQ(getValue(2, 17.0) - getValue(2, 2.0), outDataD->GetValue(2));
    }
}

TEST_F(TemporalDifferenceFilter_test, DifferenceInverted)
{
    auto source = createSource(timeSteps());
    vtkNew<TemporalDifferenceFilter> filter;
    filter->SetInputConnection(source->GetOutputPort());

    filter->SetTimeStep0(2.0);
    filter->SetTimeStep1(1.0);
    ASSERT_TRUE(filter->GetExecutive()->Update());
    auto output = filter->GetOutput();
    ASSERT_TRUE(output);
    {
        auto outData = output->GetPointData()->GetArray("temp");
        ASSERT_TRUE(outData);
        auto outDataD = vtkDoubleArray::SafeDownCast(outData);
        ASSERT_TRUE(outDataD);

        ASSERT_DOUBLE_EQ(getValue(0, 1.0) - getValue(0, 2.0), outDataD->GetValue(0));
        ASSERT_DOUBLE_EQ(getValue(1, 1.0) - getValue(1, 2.0), outDataD->GetValue(1));
        ASSERT_DOUBLE_EQ(getValue(2, 1.0) - getValue(2, 2.0), outDataD->GetValue(2));
    }

    {
        filter->SetTimeStep0(17.0);
        filter->SetTimeStep1(2.0);
        ASSERT_TRUE(filter->GetExecutive()->Update());
        auto outDataD = vtkDoubleArray::SafeDownCast(output->GetPointData()->GetArray("temp"));
        ASSERT_TRUE(outDataD);

        ASSERT_DOUBLE_EQ(getValue(0, 2.0) - getValue(0, 17.0), outDataD->GetValue(0));
        ASSERT_DOUBLE_EQ(getValue(1, 2.0) - getValue(1, 17.0), outDataD->GetValue(1));
        ASSERT_DOUBLE_EQ(getValue(2, 2.0) - getValue(2, 17.0), outDataD->GetValue(2));
    }
}

TEST_F(TemporalDifferenceFilter_test, PassThroughNoTemporal)
{
    auto source = createSource(timeSteps());
    auto input = vtkDataSet::SafeDownCast(source->GetInput());
    vtkNew<vtkDoubleArray> noTemporalArray;
    noTemporalArray->SetName("noTemporal");
    input->GetPointData()->AddArray(noTemporalArray.Get());

    vtkNew<TemporalDifferenceFilter> filter;
    filter->SetInputConnection(source->GetOutputPort());

    filter->SetTimeStep0(1.0);
    filter->SetTimeStep1(2.0);
    ASSERT_TRUE(filter->GetExecutive()->Update());
    auto output = filter->GetOutput();
    ASSERT_TRUE(output);
    auto passedThrough = output->GetPointData()->GetArray("noTemporal");

    ASSERT_EQ(noTemporalArray.Get(), passedThrough);
}

TEST_F(TemporalDifferenceFilter_test, PreserveAttributes)
{
    auto source = createSource(timeSteps());
    auto input = vtkDataSet::SafeDownCast(source->GetInput());

    vtkNew<vtkDoubleArray> noTemporalArray;
    noTemporalArray->SetName("noTemporal");
    noTemporalArray->SetNumberOfComponents(3);
    noTemporalArray->SetNumberOfTuples(3);
    input->GetPointData()->SetVectors(noTemporalArray.Get());

    vtkNew<TemporalDifferenceFilter> filter;
    filter->SetInputConnection(source->GetOutputPort());

    filter->SetTimeStep0(1.0);
    filter->SetTimeStep1(2.0);
    ASSERT_TRUE(filter->GetExecutive()->Update());
    auto output = filter->GetOutput();
    ASSERT_TRUE(output);
    auto outScalarsTemporal = output->GetPointData()->GetScalars("temp");
    auto outVectorsNonTemporal = output->GetPointData()->GetScalars("noTemporal");

    ASSERT_TRUE(outScalarsTemporal);
    ASSERT_TRUE(outVectorsNonTemporal);
}
