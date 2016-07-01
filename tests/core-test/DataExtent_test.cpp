#include <gtest/gtest.h>

#include <core/utility/DataExtent.h>


using Bounds1 = DataExtent<float, 1u>;
using Bounds2 = DataExtent<float, 2u>;
using Vec2 = vtkVector2f;


TEST(DataExtent_test, ctors)
{
#define values 0, 1, 2, 3, 4, 5
    const auto ext1 = ImageExtent({ values });
    const auto ext2 = ImageExtent(ImageExtent::array_t({ values }));
    const ImageExtent::value_type ext3[6] = { values };

    /** Incompatible with Visual Studio 2013 Compiler :( */
    //const auto ext4 = ImageExtent(values);
#undef values

    ASSERT_EQ(ext1, ext2);
    ASSERT_EQ(ext1, ext3);
    //ASSERT_EQ(ext1, ext4);
}

TEST(DataExtent_test, intersection)
{
    ASSERT_EQ(Bounds2({1, 2, 1, 2}), Bounds2({ 0, 2, 0, 2 }).intersect(Bounds2({ 1, 3, 1, 3 })));
    ASSERT_TRUE(Bounds2({ 1, 2, 1, 2 }).intersect(Bounds2({ 3, 4, 3, 4 })).isEmpty());
}

TEST(DataExtent_test, contains_point)
{
    ASSERT_TRUE(Bounds2({ 0.f, 1.f, 0.f, 1.f }).contains(Vec2{ 0.5f, 0.5f }));
    ASSERT_TRUE(Bounds2({ 0.f, 1.f, 0.f, 1.f }).contains(Vec2{ 0.f, 0.f }));
    ASSERT_TRUE(Bounds2({ 0.f, 1.f, 0.f, 1.f }).contains(Vec2{ 1.f, 1.f }));
    ASSERT_TRUE(Bounds2({ 0.f, 1.f, 0.f, 1.f }).contains(Vec2{ 0.f, 1.f }));

    ASSERT_FALSE(Bounds2({ 0.f, 1.f, 0.f, 1.f }).contains(Vec2{ -1.f, 0.f }));
    ASSERT_FALSE(Bounds2({ 0.f, 1.f, 0.f, 1.f }).contains(Vec2{ 0.f, -1.f }));
    ASSERT_FALSE(Bounds2({ 0.f, 1.f, 0.f, 1.f }).contains(Vec2{ 0.f, 2.f }));
    ASSERT_FALSE(Bounds2({ 0.f, 1.f, 0.f, 1.f }).contains(Vec2{ 2.f, 0.f }));
}

TEST(DataExtent_test, contains_value)
{
    ASSERT_TRUE(Bounds1({ 0.f, 0.f }).contains(0.f));
    ASSERT_TRUE(Bounds1({ -1.f, 1.f }).contains(-1.f));
    ASSERT_TRUE(Bounds1({ 0.f, 1.f }).contains(0.5f));
    ASSERT_TRUE(Bounds1({ 0.f, 1.f }).contains(1.f));

    ASSERT_FALSE(Bounds1({ 0.f, 0.f }).contains(0.1f));
    ASSERT_FALSE(Bounds1({ -1.f, 1.f }).contains(-2.f));
    ASSERT_FALSE(Bounds1({ 0.f, 1.f }).contains(1.5f));
}

TEST(DataExtent_test, clampValue)
{
    ASSERT_EQ(0.f, Bounds1({ 0.f, 0.f }).clampValue(0.f));
    ASSERT_EQ(1.f, Bounds1({ 1.f, 1.f }).clampValue(2.f));
    ASSERT_EQ(1.f, Bounds1({ 1.f, 2.f }).clampValue(0.5f));
    ASSERT_EQ(2.f, Bounds1({ 1.f, 2.f }).clampValue(2.5f));
}

TEST(DataExtent_test, clampPoint)
{
    ASSERT_EQ(Vec2(0.f, 0.f), Bounds2({ 0.f, 0.f, 0.f, 0.f }).clampPoint(Vec2(0.f, 0.f)));
    ASSERT_EQ(Vec2(0.f, 0.f), Bounds2({ 0.f, 0.f, 0.f, 0.f }).clampPoint(Vec2(1.f, 1.f)));
    ASSERT_EQ(Vec2(1.f, 3.f), Bounds2({ 1.f, 2.f, 3.f, 4.f }).clampPoint(Vec2(0.f, 0.f)));
    ASSERT_EQ(Vec2(2.f, 4.f), Bounds2({ 1.f, 2.f, 3.f, 4.f }).clampPoint(Vec2(5.f, 6.f)));
}

TEST(DataExtent_test, add)
{
    ASSERT_EQ(Bounds2({ 0.f, 1.f, 0.f, 1.f }), Bounds2({ 0.f, 1.f, 0.f, 1.f }).add({}));
    ASSERT_EQ(Bounds2({ 1.f, 2.f, 1.f, 2.f }), Bounds2({ 1.f, 2.f, 1.f, 2.f }).add({}));
    ASSERT_EQ(Bounds2({ 0.f, 3.f, 0.f, 3.f }), Bounds2({ 0.f, 1.f, 0.f, 1.f }).add(Bounds2({ 2.f, 3.f, 2.f, 3.f })));
    ASSERT_EQ(Bounds2({ 0.f, 4.f, 0.f, 4.f }), Bounds2({ 0.f, 4.f, 0.f, 4.f }).add(Bounds2({ 2.f, 3.f, 2.f, 3.f })));
}

TEST(DataExtent_test, NumberOfPoints)
{
    ASSERT_EQ(0 * 1 * 1, ImageExtent({ 0, -1, 0, 0, 0, 0 }).numberOfPoints());
    ASSERT_EQ(1 * 2 * 2, ImageExtent({ 0, 0, 0, 1, 0, 1 }).numberOfPoints());
    ASSERT_EQ(2 * 2 * 2, ImageExtent({ 0, 1, 0, 1, 0, 1 }).numberOfPoints());
    ASSERT_EQ(3 * 2 * 6, ImageExtent({ 2, 4, 8, 9, 10, 15}).numberOfPoints());
}

TEST(DataExtent_test, NumberOfCells)
{
    ASSERT_EQ(0 * 0 * 0, ImageExtent({ 0, -1, 0, 0, 0, 0 }).numberOfCells());
    ASSERT_EQ(0 * 1 * 1, ImageExtent({ 0, 0, 0, 1, 0, 1 }).numberOfCells());
    ASSERT_EQ(1 * 1 * 1, ImageExtent({ 0, 1, 0, 1, 0, 1 }).numberOfCells());
    ASSERT_EQ(2 * 1 * 5, ImageExtent({ 2, 4, 8, 9, 10, 15}).numberOfCells());
}
