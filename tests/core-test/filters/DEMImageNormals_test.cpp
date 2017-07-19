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

#include <vtkExecutive.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkVector.h>

#include <core/filters/DEMImageNormals.h>
#include <core/utility/DataExtent.h>


class DEMImageNormals_test : public ::testing::Test
{
public:
    vtkSmartPointer<vtkImageData> dem;
    ImageExtent extent{ { 0, 4, 0, 5, 0, 0 } };
    vtkVector3<vtkIdType> incs;

    void SetUp() override
    {
        dem = vtkSmartPointer<vtkImageData>::New();
        dem->SetExtent(extent.data());
        dem->AllocateScalars(VTK_FLOAT, 1);
        dem->GetIncrements(incs.GetData());
        auto elevations = dem->GetPointData()->GetScalars();
        elevations->SetName("Elevations");
        for (vtkIdType i = 0; i < elevations->GetNumberOfTuples(); ++i)
        {
            elevations->SetComponent(i, 0, 0.f);
        }

        dem->SetScalarComponentFromFloat(2, 3, 0, 0, 1.0);
        dem->SetScalarComponentFromFloat(2, 4, 0, 0, 1.0);
        dem->SetScalarComponentFromFloat(2, 5, 0, 0, 2.0);
    }

    vtkIdType imgIndex(vtkIdType x, vtkIdType y, vtkIdType z = 0)
    {
        return ((x - extent[0]) * incs[0]
            + (y - extent[2]) * incs[1]
            + (z - extent[4]) * incs[2]);
    }

};

TEST_F(DEMImageNormals_test, NormalsAreNormalized)
{
    auto demNormalFilter = vtkSmartPointer<DEMImageNormals>::New();
    demNormalFilter->SetCoordinatesUnitScale(1.0);
    demNormalFilter->SetElevationUnitScale(1.0);
    demNormalFilter->SetInputData(dem);
    ASSERT_TRUE(demNormalFilter->GetExecutive()->Update());
    auto normals = demNormalFilter->GetOutput()->GetPointData()->GetNormals();
    ASSERT_TRUE(normals);
    ASSERT_EQ(dem->GetNumberOfPoints(), normals->GetNumberOfTuples());

    for (vtkIdType i = 0; i < normals->GetNumberOfTuples(); ++i)
    {
        vtkVector3d normal;
        normals->GetTuple(i, normal.GetData());
        ASSERT_FLOAT_EQ(1.f, static_cast<float>(normal.Norm()));
    }
}
