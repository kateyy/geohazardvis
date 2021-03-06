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

#include <core/utility/DataExtent.h>


using Bounds1 = DataExtent<double, 1u>;
using Bounds2 = DataExtent<double, 2u>;
using Extent2 = DataExtent<int, 2u>;
using Vec2 = vtkVector2d;


TEST(DataExtent_test, ctors)
{
#define values 0, 1, 2, 3, 4, 5
    const auto ext1 = ImageExtent({ values });
    const auto ext2 = ImageExtent(ImageExtent::array_t({ values }));
    const ImageExtent::value_type ext3[6] = { values };
    const auto ext4 = ImageExtent({
        ValueRange<ImageExtent::value_type>({ 0, 1 }),
        ValueRange<ImageExtent::value_type>({ 2, 3 }),
        ValueRange<ImageExtent::value_type>({ 4, 5 })
    });

    /** Incompatible with Visual Studio 2013 Compiler :( */
    //const auto ext5 = ImageExtent(values);
#undef values

    ASSERT_EQ(ext1, ext2);
    ASSERT_EQ(ext1, ext3);
    ASSERT_EQ(ext1, ext4);
    //ASSERT_EQ(ext1, ext5);
}

TEST(DataExtent_test, numberOfCells)
{
    ASSERT_EQ(0 * 0 * 0, ImageExtent({ 0, -1, 0, 0, 0, 0 }).numberOfCells());
    ASSERT_EQ(0 * 1 * 1, ImageExtent({ 0, 0, 0, 1, 0, 1 }).numberOfCells());
    ASSERT_EQ(1 * 1 * 1, ImageExtent({ 0, 1, 0, 1, 0, 1 }).numberOfCells());
    ASSERT_EQ(2 * 1 * 5, ImageExtent({ 2, 4, 8, 9, 10, 15}).numberOfCells());
}

TEST(DataExtent_test, operatorEqual)
{
    const auto extent1 = Extent2({ -2, 1, 3, 4 });
    const auto extent1a = Extent2({ -2, 1, 3, 4 });
    const int array1[4] = { -2, 1, 3, 4 };
    const auto extent2 = Extent2({ -2, 1, 3, 5 });
    const int array2[4] = { -2, 1, 3, 5 };

    ASSERT_TRUE(extent1 == extent1a);
    ASSERT_TRUE(extent1 == array1);
    ASSERT_FALSE(extent1 == extent2);
    ASSERT_FALSE(extent1 == array2);
}

TEST(DataExtent_test, operatorNotEqual)
{
    const auto extent1 = Extent2({ -2, 1, 3, 4 });
    const auto extent1a = Extent2({ -2, 1, 3, 4 });
    const int array1[4] = { -2, 1, 3, 4 };
    const auto extent2 = Extent2({ -2, 1, 3, 5 });
    const int array2[4] = { -2, 1, 3, 5 };

    ASSERT_FALSE(extent1 != extent1a);
    ASSERT_FALSE(extent1 != array1);
    ASSERT_TRUE(extent1 != extent2);
    ASSERT_TRUE(extent1 != array2);
}

TEST(DataExtent_test, center)
{
    ASSERT_EQ(vtkVector2d(2.0, -1.0), Bounds2({0.0, 4.0, -4.0, 2.0}).center());
    ASSERT_EQ(vtkVector2d(2.0, 0.0), Bounds2({ 2.0, 2.0, 0.0, 0.0 }).center());
}

TEST(DataExtent_test, center_value)
{
    ASSERT_EQ(2.0, ValueRange<>({ 0.0, 4.0 }).center());
    ASSERT_EQ(-2.0, ValueRange<>({ -3.0, -1.0 }).center());
    ASSERT_EQ(0.0, ValueRange<>({ 0.0, 0.0 }).center());
}

TEST(DataExtent_test, componentSize)
{
    ASSERT_EQ(vtkVector2d(4.0, 3.0), Bounds2({ 0.0, 4.0, -4.0, -1.0 }).componentSize());
    ASSERT_EQ(vtkVector2d(0.0, 0.0), Bounds2({ 2.0, 2.0, 0.0, 0.0 }).componentSize());
}

TEST(DataExtent_test, componentSize_value)
{
    ASSERT_EQ(4.0, ValueRange<>({ 0.0, 4.0 }).componentSize());
    ASSERT_EQ(2.0, ValueRange<>({ -3.0, -1.0 }).componentSize());
    ASSERT_EQ(0.0, ValueRange<>({ 0.0, 0.0 }).componentSize());
}

TEST(DataExtent_test, min)
{
    ASSERT_EQ(vtkVector2d(0.0, -4.0), Bounds2({ 0.0, 4.0, -4.0, -1.0 }).min());
    ASSERT_EQ(vtkVector2d(2.0, 0.0), Bounds2({ 2.0, 2.0, 0.0, 0.0 }).min());
}

TEST(DataExtent_test, min_value)
{
    ASSERT_EQ(0.0, ValueRange<>({ 0.0, 4.0 }).min());
    ASSERT_EQ(-3.0, ValueRange<>({ -3.0, -1.0 }).min());
}

TEST(DataExtent_test, max)
{
    ASSERT_EQ(vtkVector2d(4.0, -1.0), Bounds2({ 0.0, 4.0, -4.0, -1.0 }).max());
    ASSERT_EQ(vtkVector2d(2.0, 0.0), Bounds2({ 2.0, 2.0, 0.0, 0.0 }).max());
}

TEST(DataExtent_test, max_value)
{
    ASSERT_EQ(4.0, ValueRange<>({ 0.0, 4.0 }).max());
    ASSERT_EQ(-1.0, ValueRange<>({ -3.0, -1.0 }).max());
}

TEST(DataExtent_test, extractDimension)
{
    ASSERT_EQ(ValueRange<>({ 0.0, 4.0 }), Bounds2({ 0.0, 4.0, -4.0, -1.0 }).extractDimension(0));
    ASSERT_EQ(ValueRange<>({ -4.0, -1.0 }), Bounds2({ 0.0, 4.0, -4.0, -1.0 }).extractDimension(1));
}

TEST(DataExtent_test, extractDimension_value)
{
    ASSERT_EQ(ValueRange<>({ 2.0, 3.0 }), ValueRange<>({ 2.0, 3.0 }).extractDimension(0));
}

TEST(DataExtent_test, setDimension)
{
    auto bounds = Bounds2({ 0.0, 4.0, -4.0, -1.0 });
    ASSERT_EQ(Bounds2({ -2.0, -1.0, -4.0, -1.0 }), bounds.setDimension(0, -2.0, -1.0));
    ASSERT_EQ(Bounds2({ -2.0, -1.0, -4.0, -1.0 }), bounds);
    ASSERT_EQ(Bounds2({ -2.0, -1.0, 4.0, 5.0 }), bounds.setDimension(1, ValueRange<>({ 4.0, 5.0 })));
    ASSERT_EQ(Bounds2({ -2.0, -1.0, 4.0, 5.0 }), bounds);
}

TEST(DataExtent_test, setDimensions_value)
{
    auto range = ValueRange<>({ -2, 5 });
    ASSERT_EQ(ValueRange<>({ -3, 7 }), range.setDimension(0, -3, 7));
    ASSERT_EQ(ValueRange<>({ -3, 7 }), range);
}

TEST(DataExtent_test, convertTo_lessDimensions)
{
    ASSERT_EQ(Bounds2({ -2, 4, -3, 5 }), DataBounds({ -2, 4, -3, 5, 6, 7 }).convertTo<2>());
}

TEST(DataExtent_test, convertTo_moreDimensions)
{
    const auto bounds = Bounds2({ -2, 4, -3, 5 }).convertTo<3>();
    const auto emptyRange = ValueRange<>();

    ASSERT_EQ(DataBounds({ -2, 4, -3, 5, emptyRange[0], emptyRange[1] }), bounds);
    ASSERT_TRUE(bounds.isEmpty());
}

TEST(DataExtent_test, convertTo_type)
{
    const auto intBounds = DataExtent<int, 2>({ -2, 4, -3, 5 });
    ASSERT_EQ(intBounds, Bounds2({ -2, 4, -3, 5 }).convertTo<int>());
}

TEST(DataExtent_test, add)
{
    ASSERT_EQ(Bounds2({ 0., 1., 0., 1. }), Bounds2({ 0., 1., 0., 1. }).add(Bounds2{}));
    ASSERT_EQ(Bounds2({ 1., 2., 1., 2. }), Bounds2({ 1., 2., 1., 2. }).add(Bounds2{}));
    ASSERT_EQ(Bounds2({ 0., 3., 0., 3. }), Bounds2({ 0., 1., 0., 1. }).add(Bounds2({ 2., 3., 2., 3. })));
    ASSERT_EQ(Bounds2({ 0., 4., 0., 4. }), Bounds2({ 0., 4., 0., 4. }).add(Bounds2({ 2., 3., 2., 3. })));
}

TEST(DataExtent_test, sum)
{
    ASSERT_EQ(Bounds2({ 0., 1., 0., 1. }), Bounds2({ 0., 1., 0., 1. }).sum({}));
    ASSERT_EQ(Bounds2({ 1., 2., 1., 2. }), Bounds2({ 1., 2., 1., 2. }).sum({}));
    ASSERT_EQ(Bounds2({ 0., 3., 0., 3. }), Bounds2({ 0., 1., 0., 1. }).sum(Bounds2({ 2., 3., 2., 3. })));
    ASSERT_EQ(Bounds2({ 0., 4., 0., 4. }), Bounds2({ 0., 4., 0., 4. }).sum(Bounds2({ 2., 3., 2., 3. })));
}

TEST(DataExtent_test, add_value)
{
    ASSERT_EQ(ValueRange<>({ 0., 1. }), ValueRange<>({ 0., 1. }).add(1.0));
    ASSERT_EQ(ValueRange<>({ 0., 3. }), ValueRange<>({ 0., 1. }).add(3.0));
    ASSERT_EQ(ValueRange<>({ 0., 4. }), ValueRange<>({ 2., 4. }).add(0.0));
}

TEST(DataExtent_test, add_point)
{
    ASSERT_EQ(Bounds2({ 0., 1., 0., 1. }), Bounds2({ 0., 1., 0., 1. }).add(Vec2{ 0.0, 1.0 }));
    ASSERT_EQ(Bounds2({ 0., 3., 0., 3. }), Bounds2({ 0., 1., 0., 1. }).add(Vec2{ 3.0, 3.0 }));
    ASSERT_EQ(Bounds2({ 2., 4., -1., 4. }), Bounds2({ 2., 4., 1., 4. }).add(Vec2{ 2.0, -1.0 }));
}

TEST(DataExtent_test, intersect)
{
    ASSERT_EQ(Bounds2({ 1, 2, 1, 2 }), Bounds2({ 0, 2, 0, 2 }).intersect(Bounds2({ 1, 3, 1, 3 })));
    ASSERT_TRUE(Bounds2({ 1, 2, 1, 2 }).intersect(Bounds2({ 3, 4, 3, 4 })).isEmpty());
}

TEST(DataExtent_test, intersection)
{
    ASSERT_EQ(Bounds2({ 1, 2, 1, 2 }), Bounds2({ 0, 2, 0, 2 }).intersection(Bounds2({ 1, 3, 1, 3 })));
    ASSERT_TRUE(Bounds2({ 1, 2, 1, 2 }).intersection(Bounds2({ 3, 4, 3, 4 })).isEmpty());
}

TEST(DataExtent_test, shift_ValueRange)
{
    auto valueRange = ValueRange<>({ -4, -3 });
    valueRange.shift(5);
    ASSERT_EQ(ValueRange<>({ 1, 2 }), valueRange);
}

TEST(DataExtent_test, shift_Bounds)
{
    auto bounds = Bounds2({ -4, -3, 3, 5 });
    bounds.shift(vtkVector2d{ 5, -6 });
    ASSERT_EQ(Bounds2({ 1, 2, -3, -1 }), bounds);
}

TEST(DataExtent_test, shifted_ValueRange)
{
    const auto valueRange = ValueRange<>({ -4, -3 });
    ASSERT_EQ(ValueRange<>({ 1, 2 }), valueRange.shifted(5));
}

TEST(DataExtent_test, shifted_Bounds)
{
    const auto bounds = Bounds2({ -4, -3, 3, 5 });
    ASSERT_EQ(Bounds2({ 1, 2, -3, -1 }), bounds.shifted(vtkVector2d{ 5, -6 }));
}

TEST(DataExtent_test, scale_ValueRange)
{
    auto valueRange = ValueRange<>({ -4, -3 });
    valueRange.scale(-0.5);
    ASSERT_EQ(ValueRange<>({ 2, 1.5 }), valueRange);
}

TEST(DataExtent_test, scale_Bounds)
{
    auto bounds = Bounds2({ -4, -3, 3, 5 });
    bounds.scale(vtkVector2d{ -2, 3 });
    ASSERT_EQ(Bounds2({ 8, 6, 9, 15 }), bounds);
}

TEST(DataExtent_test, scaled_ValueRange)
{
    const auto valueRange = ValueRange<>({ -4, -3 });
    ASSERT_EQ(ValueRange<>({ 2, 1.5 }), valueRange.scaled(-0.5));
}

TEST(DataExtent_test, scaled_Bounds)
{
    const auto bounds = Bounds2({ -4, -3, 3, 5 });
    ASSERT_EQ(Bounds2({ 8, 6, 9, 15 }), bounds.scaled(vtkVector2d{ -2, 3 }));
}

TEST(DataExtent_test, contains_point)
{
    ASSERT_TRUE(Bounds2({ 0., 1., 0., 1. }).contains(Vec2{ 0.5f, 0.5f }));
    ASSERT_TRUE(Bounds2({ 0., 1., 0., 1. }).contains(Vec2{ 0., 0. }));
    ASSERT_TRUE(Bounds2({ 0., 1., 0., 1. }).contains(Vec2{ 1., 1. }));
    ASSERT_TRUE(Bounds2({ 0., 1., 0., 1. }).contains(Vec2{ 0., 1. }));

    ASSERT_FALSE(Bounds2({ 0., 1., 0., 1. }).contains(Vec2{ -1., 0. }));
    ASSERT_FALSE(Bounds2({ 0., 1., 0., 1. }).contains(Vec2{ 0., -1. }));
    ASSERT_FALSE(Bounds2({ 0., 1., 0., 1. }).contains(Vec2{ 0., 2. }));
    ASSERT_FALSE(Bounds2({ 0., 1., 0., 1. }).contains(Vec2{ 2., 0. }));
}

TEST(DataExtent_test, contains_value)
{
    ASSERT_TRUE(Bounds1({ 0., 0. }).contains(0.));
    ASSERT_TRUE(Bounds1({ -1., 1. }).contains(-1.));
    ASSERT_TRUE(Bounds1({ 0., 1. }).contains(0.5f));
    ASSERT_TRUE(Bounds1({ 0., 1. }).contains(1.));

    ASSERT_FALSE(Bounds1({ 0., 0. }).contains(0.1f));
    ASSERT_FALSE(Bounds1({ -1., 1. }).contains(-2.));
    ASSERT_FALSE(Bounds1({ 0., 1. }).contains(1.5f));
}

TEST(DataExtent_test, clampValue)
{
    ASSERT_EQ(0., Bounds1({ 0., 0. }).clampValue(0.));
    ASSERT_EQ(1., Bounds1({ 1., 1. }).clampValue(2.));
    ASSERT_EQ(1., Bounds1({ 1., 2. }).clampValue(0.5f));
    ASSERT_EQ(2., Bounds1({ 1., 2. }).clampValue(2.5f));
}

TEST(DataExtent_test, clampPoint)
{
    ASSERT_EQ(Vec2(0., 0.), Bounds2({ 0., 0., 0., 0. }).clampPoint(Vec2(0., 0.)));
    ASSERT_EQ(Vec2(0., 0.), Bounds2({ 0., 0., 0., 0. }).clampPoint(Vec2(1., 1.)));
    ASSERT_EQ(Vec2(1., 3.), Bounds2({ 1., 2., 3., 4. }).clampPoint(Vec2(0., 0.)));
    ASSERT_EQ(Vec2(2., 4.), Bounds2({ 1., 2., 3., 4. }).clampPoint(Vec2(5., 6.)));
}

TEST(DataExtent_test, numberOfPoints)
{
    ASSERT_EQ(0 * 1 * 1, ImageExtent({ 0, -1, 0, 0, 0, 0 }).numberOfPoints());
    ASSERT_EQ(1 * 2 * 2, ImageExtent({ 0, 0, 0, 1, 0, 1 }).numberOfPoints());
    ASSERT_EQ(2 * 2 * 2, ImageExtent({ 0, 1, 0, 1, 0, 1 }).numberOfPoints());
    ASSERT_EQ(3 * 2 * 6, ImageExtent({ 2, 4, 8, 9, 10, 15 }).numberOfPoints());
}

TEST(DataExtent_test, relativeOriginPosition_Bounds)
{
    ASSERT_EQ(vtkVector2d({ 0.0, 0.0 }), Bounds2({ 0.0, 1.0, 0.0, 1.0 }).relativeOriginPosition());
    ASSERT_EQ(vtkVector2d({ 1.0, 1.0 }), Bounds2({ -1.0, 0.0, -1.0, 0.0 }).relativeOriginPosition());
    ASSERT_EQ(vtkVector2d({ 0.25, 0.5 }), Bounds2({ -2.0, 6.0, -4.0, 4.0 }).relativeOriginPosition());
    ASSERT_EQ(vtkVector2d({ 2.0, 4.0 }), Bounds2({ -2.0, -1.0, -4.0, -3.0 }).relativeOriginPosition());
    ASSERT_EQ(vtkVector2d({ -1.0, -1.0 }), Bounds2({ 1.0, 2.0, 2.0, 4.0 }).relativeOriginPosition());
}

TEST(DataExtent_test, relativeOriginPosition_ValueRange)
{
    ASSERT_EQ(0.0, ValueRange<>({ 0.0, 1.0 }).relativeOriginPosition());
    ASSERT_EQ(1.0, ValueRange<>({ -1.0, 0.0 }).relativeOriginPosition());
    ASSERT_EQ(0.25, ValueRange<>({ -2.0, 6.0 }).relativeOriginPosition());
    ASSERT_EQ(2.0, ValueRange<>({ -2.0, -1.0 }).relativeOriginPosition());
    ASSERT_EQ(-1.0, ValueRange<>({ 1.0, 2.0 }).relativeOriginPosition());
}
