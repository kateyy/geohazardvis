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

#include <vtkCellData.h>
#include <vtkExecutive.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <core/filters/AssignPointAttributeToCoordinatesFilter.h>



class AssignPointAttributeToCoordinatesFilter_test : public ::testing::Test
{
public:
    void SetUp() override
    {
        inDs = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->SetDataTypeToFloat();
        points->SetNumberOfPoints(5);
        points->GetData()->SetName(initialPointsName.c_str());
        initialPoints = vtkFloatArray::FastDownCast(points->GetData());
        inDs->SetPoints(points);

        filter = vtkSmartPointer<AssignPointAttributeToCoordinatesFilter>::New();
        filter->SetInputData(inDs);

        newPoints = vtkSmartPointer<vtkFloatArray>::New();
        newPoints->SetNumberOfComponents(3);
        newPoints->SetNumberOfTuples(5);
        newPoints->SetName(newPointsName.c_str());
        inDs->GetPointData()->AddArray(newPoints);

        auto someAttribute = vtkSmartPointer<vtkFloatArray>::New();
        someAttribute->SetName(someAttributeName.c_str());
        inDs->GetPointData()->AddArray(someAttribute);
        inDs->GetCellData()->AddArray(someAttribute);
        inDs->GetFieldData()->AddArray(someAttribute);
    }

    const std::string initialPointsName = "InitialPoints";
    const std::string newPointsName = "NewPoints";
    const std::string someAttributeName = "SomeAttribute";

    vtkSmartPointer<vtkPolyData> inDs;
    vtkSmartPointer<vtkFloatArray> initialPoints;
    vtkSmartPointer<vtkFloatArray> newPoints;
    vtkSmartPointer<AssignPointAttributeToCoordinatesFilter> filter;
};

TEST_F(AssignPointAttributeToCoordinatesFilter_test, DoAssign)
{
    filter->SetAttributeArrayToAssign(newPointsName);
    ASSERT_TRUE(filter->GetExecutive()->Update());
    ASSERT_EQ(newPoints.Get(), filter->GetOutput()->GetPoints()->GetData());
}

TEST_F(AssignPointAttributeToCoordinatesFilter_test, PassPreviousPoints)
{
    filter->SetAttributeArrayToAssign(newPointsName);
    ASSERT_TRUE(filter->GetExecutive()->Update());
    ASSERT_TRUE(filter->GetOutput()->GetPointData()->GetArray(initialPointsName.c_str()));
}

TEST_F(AssignPointAttributeToCoordinatesFilter_test, PreserveInput)
{
    filter->SetAttributeArrayToAssign(newPointsName);
    ASSERT_TRUE(filter->GetExecutive()->Update());
    ASSERT_EQ(initialPoints.Get(), inDs->GetPoints()->GetData());
    ASSERT_EQ(newPoints.Get(), inDs->GetPointData()->GetArray(newPointsName.c_str()));
}

TEST_F(AssignPointAttributeToCoordinatesFilter_test, PassOtherAttributes)
{
    filter->SetAttributeArrayToAssign(newPointsName);
    ASSERT_TRUE(filter->GetExecutive()->Update());
    ASSERT_TRUE(inDs->GetPointData()->GetArray(someAttributeName.c_str()));
    ASSERT_TRUE(inDs->GetCellData()->GetArray(someAttributeName.c_str()));
    ASSERT_TRUE(inDs->GetFieldData()->GetArray(someAttributeName.c_str()));
}
