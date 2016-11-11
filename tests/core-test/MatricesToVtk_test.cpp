#include <gtest/gtest.h>

#include <array>
#include <cassert>

#include <vtkImageData.h>

#include <core/data_objects/DataObject.h>
#include <core/io/MatricesToVtk.h>
#include <core/utility/DataExtent_print.h>


void PrintTo(const DataBounds & bounds, std::ostream * os)
{
    *os << bounds;
}


namespace
{
    enum Axis : int
    {
        aX = 0, aY = 1, aZ = 2
    };
    struct AxesOrders
    {
        AxesOrders(Axis _0, Axis _1, Axis _2)
            : _0 {_0}, _1 {_1}, _2 {_2}
        {
            assert(_0 != _1 && _1 != _2);
        }
        const Axis _0, _1, _2;
    };

    std::ostream & operator<<(std::ostream & os, const AxesOrders & axesOrders)
    {
        os << static_cast<char>('x' + axesOrders._0)
           << static_cast<char>('x' + axesOrders._1)
           << static_cast<char>('x' + axesOrders._2);
        return os;
    }
}


class MatricesToVtk_Grid3D_test : public testing::TestWithParam<AxesOrders>
{
public:

    void generateVectors(io::InputVector & data, size_t firstColumn)
    {
        assert(data.size() >= firstColumn + 3);

        const size_t numValues = data[firstColumn].size();
        assert(data[firstColumn + 1].size() == numValues);
        assert(data[firstColumn + 2].size() == numValues);

        for (size_t axis = 0; axis < 3; ++axis)
        {
            for (size_t i = 0; i < numValues; ++i)
            {
                data[firstColumn + axis][i] = static_cast<io::t_FP>(axis + 1) + static_cast<io::t_FP>(i + 1) / 10.0;
            }
        }
    }

    io::InputVector generateGrid(AxesOrders orders)
    {
        io::InputVector columns = {
            { 0, 1, 0, 1, 0, 1, 0, 1 },
            { 0, 0, 2, 2, 0, 0, 2, 2 },
            { 0, 0, 0, 0, 3, 3, 3, 3 }};
        io::InputVector data(6);
        data[0] = std::move(columns[static_cast<size_t>(orders._0)]);
        data[1] = std::move(columns[static_cast<size_t>(orders._1)]);
        data[2] = std::move(columns[static_cast<size_t>(orders._2)]);

        if (orders._0 > orders._1)
        {
            std::swap(data[0], data[1]);
        }
        if (orders._1 > orders._2)
        {
            std::swap(data[1], data[2]);
        }
        if (orders._0 > orders._1)
        {
            std::swap(data[0], data[1]);
        }

        data[3].resize(data[0].size());
        data[4].resize(data[0].size());
        data[5].resize(data[0].size());
        generateVectors(data, 3);

        return data;
    }
};


TEST_P(MatricesToVtk_Grid3D_test, DISABLED_loadGrid3D)
{
    const auto & param = GetParam();

    io::ReadDataSet readDataSet;
    readDataSet.type = io::DataSetType::vectorGrid3D;
    auto & data = readDataSet.data;
    data = generateGrid(param);

    auto dataObject = MatricesToVtk::loadGrid3D("AGrid", { readDataSet });
    ASSERT_TRUE(dataObject);
    auto image = vtkImageData::SafeDownCast(dataObject->dataSet());
    ASSERT_TRUE(image);

    std::array<int, 3> dimensions;
    image->GetDimensions(dimensions.data());
    ASSERT_EQ(2, dimensions[0]);
    ASSERT_EQ(2, dimensions[1]);
    ASSERT_EQ(2, dimensions[2]);

    const DataBounds expectedBounds({
        0.0, static_cast<io::t_FP>(param._0) + 1.0,
        0.0, static_cast<io::t_FP>(param._1) + 1.0,
        0.0, static_cast<io::t_FP>(param._2) + 1.0
    });
    DataBounds bounds;
    image->GetBounds(bounds.data());
    ASSERT_EQ(expectedBounds, bounds);
}

INSTANTIATE_TEST_CASE_P(Grid3D_axesOrders, MatricesToVtk_Grid3D_test,
    ::testing::Values(AxesOrders(aX, aY, aZ), AxesOrders(aX, aZ, aY),
                      AxesOrders(aY, aX, aZ), AxesOrders(aY, aZ, aX),
                      AxesOrders(aZ, aX, aY), AxesOrders(aZ, aY, aX)));
