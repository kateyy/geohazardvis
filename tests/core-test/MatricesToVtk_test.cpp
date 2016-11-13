#include <gtest/gtest.h>

#include <array>
#include <cassert>

#include <vtkImageData.h>
#include <vtkPointData.h>

#include <core/data_objects/DataObject.h>
#include <core/io/MatricesToVtk.h>
#include <core/utility/DataExtent_print.h>


/** Workaround for gtest not selecting the expected PrintTo or operator<<
  * (Cannot work for template classes)
  * https://stackoverflow.com/questions/25146997/teach-google-test-how-to-print-eigen-matrix
*/
class tDataBounds : public DataBounds
{
public:
    using DataBounds::DataBounds;
};


void PrintTo(const tDataBounds & bounds, std::ostream * os)
{
    *os << bounds;
}

void PrintTo(const vtkVector3d & vector, std::ostream * os)
{
    *os << vector.GetX() << ":" << vector.GetY() << ":" << vector.GetZ();
}

namespace
{

enum Axis : int
{
    aX = 0, aY = 1, aZ = 2
};

/** Represent axes ordered as they are stored in the data set.
    * E.g., YZX means y values with same z and x value are stored consecutively in the data set,
    * followed by data points for all y values with the next z value.
    * A regular grid data set such as vtkImageData is assumed. vtkImageData stores it data values
    * in XYZ order (using the above definition). */
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
    os  << static_cast<char>('x' + axesOrders._0)
        << static_cast<char>('x' + axesOrders._1)
        << static_cast<char>('x' + axesOrders._2);
    return os;
}

}


class MatricesToVtk_Grid3D_test : public ::testing::Test
{
};

class MatricesToVtk_Grid3D_ptest :
    public MatricesToVtk_Grid3D_test,
    public ::testing::WithParamInterface<AxesOrders>
{
public:

    void generateVectors(io::InputVector & data, size_t colCountBegin, size_t colCountEnd)
    {
        assert(data.size() >= colCountEnd && colCountBegin <= colCountEnd);
        if (colCountBegin == colCountEnd)
        {
            return;
        }

        const size_t numValues = data[colCountBegin].size();
        for (auto && col : data)
        {
            assert(col.size() == numValues);
        }

        for (size_t c = colCountBegin; c < colCountEnd; ++c)
        {
            for (size_t i = 0; i < numValues; ++i)
            {
                data[colCountBegin][i] = static_cast<io::t_FP>(c + 1) + static_cast<io::t_FP>(i + 1) / 10.0;
            }
        }
    }

    io::InputVector generateGrid(AxesOrders orders, size_t vectorComponents = 0u)
    {
        io::InputVector columnsByOrder = {
            { 0, 1, 0, 1, 0, 1, 0, 1 },
            { 0, 0, 2, 2, 0, 0, 2, 2 },
            { 0, 0, 0, 0, 3, 3, 3, 3 }};
        io::InputVector data(3u + vectorComponents);
        // Construct point columns with orders as requested
        data[static_cast<size_t>(orders._0)] = std::move(columnsByOrder[0]);
        data[static_cast<size_t>(orders._1)] = std::move(columnsByOrder[1]);
        data[static_cast<size_t>(orders._2)] = std::move(columnsByOrder[2]);

        for (size_t i = 0; i < vectorComponents; ++i)
        {
            data[3u + i].resize(data[0].size());
        }
        generateVectors(data, 3u, 3u + vectorComponents);

        return data;
    }
};

TEST_F(MatricesToVtk_Grid3D_test, read_points_from_vtkImageData)
{
    auto image = vtkSmartPointer<vtkImageData>::New();
    image->SetExtent(0, 2, 0, 3, 0, 2);
    image->SetSpacing(0.1, 0.1, 0.1);
    image->SetOrigin(-1.0, -2.0, -3.0);

    io::ReadDataSet readDataSet;
    readDataSet.type = io::DataSetType::vectorGrid3D;
    auto & data = readDataSet.data;

    const vtkIdType numPoints = image->GetNumberOfPoints();
    // xyz (tested) + 3-component vector (not tested here)
    data.resize(6, std::vector<io::t_FP>(static_cast<size_t>(numPoints)));
    for (vtkIdType i = 0; i < numPoints; ++i)
    {
        const auto stdI = static_cast<size_t>(i);
        vtkVector3d point;
        image->GetPoint(i, point.GetData());
        data[0u][stdI] = point.GetX();
        data[1u][stdI] = point.GetY();
        data[2u][stdI] = point.GetZ();
    }

    auto parsedImage = MatricesToVtk::loadGrid3D("TestingGrid3D", { readDataSet });
    ASSERT_TRUE(parsedImage);
    ASSERT_TRUE(parsedImage->dataSet());
    ASSERT_EQ(numPoints, parsedImage->dataSet()->GetNumberOfPoints());
    for (vtkIdType i = 0; i < numPoints; ++i)
    {
        vtkVector3d pRef, pParsed;
        image->GetPoint(i, pRef.GetData());
        parsedImage->dataSet()->GetPoint(i, pParsed.GetData());
        ASSERT_DOUBLE_EQ(pRef.GetX(), pRef.GetX());
        ASSERT_DOUBLE_EQ(pRef.GetY(), pRef.GetY());
        ASSERT_DOUBLE_EQ(pRef.GetZ(), pRef.GetZ());
    }
}

TEST_P(MatricesToVtk_Grid3D_ptest, loadGrid3D)
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

    tDataBounds expectedBounds;
    expectedBounds.setDimension(static_cast<size_t>(param._0), ValueRange<>({0.0, 1.0}));
    expectedBounds.setDimension(static_cast<size_t>(param._1), ValueRange<>({0.0, 2.0}));
    expectedBounds.setDimension(static_cast<size_t>(param._2), ValueRange<>({0.0, 3.0}));
    tDataBounds bounds;
    image->GetBounds(bounds.data());
    ASSERT_EQ(expectedBounds, bounds);
}

TEST_P(MatricesToVtk_Grid3D_ptest, loadGrid3D_vector3)
{
    const auto & param = GetParam();

    io::ReadDataSet readDataSet;
    readDataSet.type = io::DataSetType::vectorGrid3D;
    auto & data = readDataSet.data;
    data = generateGrid(param, 3u);

    auto dataObject = MatricesToVtk::loadGrid3D("AGrid", { readDataSet });
    ASSERT_TRUE(dataObject);
    auto image = vtkImageData::SafeDownCast(dataObject->dataSet());
    ASSERT_TRUE(image);

    std::array<int, 3> dimensions;
    image->GetDimensions(dimensions.data());
    ASSERT_EQ(2, dimensions[0]);
    ASSERT_EQ(2, dimensions[1]);
    ASSERT_EQ(2, dimensions[2]);

    auto vectors = image->GetPointData()->GetVectors();
    ASSERT_TRUE(vectors);

    std::array<int, 3> refIncrements;
    refIncrements[static_cast<size_t>(param._0)] = 1;
    refIncrements[static_cast<size_t>(param._1)] = dimensions[0];
    refIncrements[static_cast<size_t>(param._2)] = dimensions[0] * dimensions[1];

    for (int z = 0; z < dimensions[2]; ++z)
    {
        for (int y = 0; y < dimensions[1]; ++y)
        {
            for (int x = 0; x < dimensions[0]; ++x)
            {
                const size_t refIndex = static_cast<size_t>(
                    refIncrements[0] * x + refIncrements[1] * y + refIncrements[2] * z);
                for (int c = 0; c < 3; ++c)
                {
                    const auto expected = static_cast<float>(
                        data[3u + static_cast<size_t>(c)][refIndex]);
                    const auto parsedComponent = image->GetScalarComponentAsFloat(x, y, z, c);
                    ASSERT_FLOAT_EQ(expected, parsedComponent);
                }
            }
        }
    }
}

INSTANTIATE_TEST_CASE_P(Grid3D_axesOrders, MatricesToVtk_Grid3D_ptest,
    ::testing::Values(AxesOrders(aX, aY, aZ), AxesOrders(aX, aZ, aY),
                      AxesOrders(aY, aX, aZ), AxesOrders(aY, aZ, aX),
                      AxesOrders(aZ, aX, aY), AxesOrders(aZ, aY, aX)));
