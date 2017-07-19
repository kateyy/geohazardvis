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

#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRearrangeFields.h>
#include <vtkSmartPointer.h>


TEST(vtkRearrangeFields_test, assign_to_same_attribute_removes_attribute_flag)
{
    auto dataSet = vtkSmartPointer<vtkPolyData>::New();
    auto normals = vtkSmartPointer<vtkFloatArray>::New();
    normals->SetName("someNormals");
    normals->SetNumberOfComponents(3);
    dataSet->GetPointData()->SetNormals(normals);

    auto rearrangeFields = vtkSmartPointer<vtkRearrangeFields>::New();
    rearrangeFields->AddOperation(vtkRearrangeFields::MOVE,
        vtkDataSetAttributes::NORMALS, 
        vtkRearrangeFields::POINT_DATA,
        vtkRearrangeFields::POINT_DATA);
    rearrangeFields->SetInputData(dataSet);
    rearrangeFields->Update();

    auto outputDataSet = rearrangeFields->GetOutput();

    ASSERT_EQ(nullptr, outputDataSet->GetPointData()->GetNormals());
}
