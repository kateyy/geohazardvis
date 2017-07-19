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

#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPlot.h>
#include <vtkPointData.h>
#include <vtkTable.h>

#include <cmath>
#include <limits>

#include <core/context2D_data/DataProfile2DContextPlot.h>
#include <core/context2D_data/vtkPlotCollection.h>
#include <core/data_objects/DataProfile2DDataObject.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/utility/DataExtent.h>


class DataProfile2DContextPlot_test : public ::testing::Test
{
public:
    template<typename ScalarType>
    std::unique_ptr<ImageDataObject> genImageWithNaNs()
    {
        auto image = vtkSmartPointer<vtkImageData>::New();
        image->SetDimensions(3, 2, 1);

        auto scalars = vtkSmartPointer<vtkAOSDataArrayTemplate<ScalarType>>::New();
        scalars->SetNumberOfTuples(image->GetNumberOfPoints());

        scalars->SetValue(0, std::numeric_limits<ScalarType>::quiet_NaN());
        scalars->SetValue(1, ScalarType(1.0));
        scalars->SetValue(2, ScalarType(2.0));
        scalars->SetValue(3, std::numeric_limits<ScalarType>::quiet_NaN());
        scalars->SetValue(4, ScalarType(1.0));
        scalars->SetValue(5, ScalarType(2.0));
        scalars->SetName("Scalars");

        image->GetPointData()->SetScalars(scalars);

        return std::make_unique<ImageDataObject>("Image", *image);
    }
};

template<typename ScalarType>
class PDataProfile2DContextPlot_test : public DataProfile2DContextPlot_test
{
};

TYPED_TEST_CASE_P(PDataProfile2DContextPlot_test);

TYPED_TEST_P(PDataProfile2DContextPlot_test, NaNValuesPassedToRendering)
{
    using ScalarType = TypeParam;

    auto image = DataProfile2DContextPlot_test::genImageWithNaNs<ScalarType>();
    const auto bounds2D = image->bounds().template convertTo<2>();
    const auto center2D = bounds2D.center();

    auto profile = std::make_unique<DataProfile2DDataObject>("Profile", *image,
        "Scalars", IndexType::points, 0);
    profile->setProfileLinePoints(
        vtkVector2d(bounds2D[0], center2D[0]),
        vtkVector2d(bounds2D[1], center2D[1]));
    auto plot = profile->createContextData();
    ASSERT_TRUE(plot);

    auto && plots = plot->plots();
    ASSERT_EQ(1, plots->GetNumberOfItems());

    auto plotItem = plots->GetLastPlot();
    ASSERT_TRUE(plotItem);

    auto table = plotItem->GetInput();
    ASSERT_TRUE(table);

    ASSERT_EQ(2, table->GetNumberOfColumns());

    auto x = table->GetColumn(0);
    auto y = vtkAOSDataArrayTemplate<ScalarType>::FastDownCast(table->GetColumn(1));

    ASSERT_TRUE(x != nullptr && y != nullptr);
    ASSERT_EQ(3, x->GetNumberOfValues());
    ASSERT_EQ(3, y->GetNumberOfValues());

    ASSERT_TRUE(std::isnan(y->GetValue(0)));
    ASSERT_EQ(ScalarType(1), y->GetValue(1));
    ASSERT_EQ(ScalarType(2), y->GetValue(2));
}

REGISTER_TYPED_TEST_CASE_P(PDataProfile2DContextPlot_test, NaNValuesPassedToRendering);

using ScalarTypes = ::testing::Types<float, double>;
INSTANTIATE_TYPED_TEST_CASE_P(FP_DataProfile2DContextPlot_test,
    PDataProfile2DContextPlot_test, ScalarTypes);
