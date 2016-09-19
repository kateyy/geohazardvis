#include <gtest/gtest.h>

#include <vtkPolyData.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/utility/DataExtent.h>


class TransformedPolyData : public PolyDataObject
{
public:
    explicit TransformedPolyData(const QString & name, vtkPolyData & dataSet)
        : PolyDataObject(name, dataSet)
    {
    }

    void setTransform(vtkTransform * transform)
    {
        m_transform = transform;
    }

protected:
    vtkSmartPointer<vtkAlgorithm> createTransformPipeline(
        const CoordinateSystemSpecification & /*toSystem*/,
        vtkAlgorithmOutput * pipelineUpstream) const override
    {
        auto filter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        filter->SetInputConnection(pipelineUpstream);
        filter->SetTransform(m_transform);
        return filter;
    }

private:
    vtkSmartPointer<vtkTransform> m_transform;
};


class CoordinateTransformableDataObject_test : public testing::Test
{
public:
    void SetUp() override
    {
    }
    void TearDown() override
    {
    }

    template<typename T = PolyDataObject>
    static std::unique_ptr<T> genPolyData()
    {
        auto poly = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();

        points->InsertNextPoint(0, 0, 0);
        points->InsertNextPoint(0, 1, 0);
        points->InsertNextPoint(1, 1, 0);
        poly->SetPoints(points);
        std::array<vtkIdType, 3> pointIds = { 0, 1, 2 };
        poly->Allocate(static_cast<vtkIdType>(pointIds.size()));
        poly->InsertNextCell(VTK_TRIANGLE, static_cast<int>(pointIds.size()), pointIds.data());

        return std::make_unique<T>("PolyData", *poly);
    }
};

TEST_F(CoordinateTransformableDataObject_test, NullTransformKeepsBounds)
{
    auto data = genPolyData<TransformedPolyData>();
    const auto dataSetBounds = DataBounds(data->dataSet()->GetBounds());
    const auto dataBounds = data->bounds();
    const auto visibleBounds = data->createRendered()->visibleBounds();

    ASSERT_EQ(dataSetBounds, dataBounds);
    ASSERT_EQ(dataBounds, visibleBounds);
}

TEST_F(CoordinateTransformableDataObject_test, TransformAppliedToVisibleBounds)
{
    const auto coordsSpec = ReferencedCoordinateSystemSpecification(
        CoordinateSystemType::geographic,
        "testSystem",
        {}, {}, {});
    const auto targetCoordsSpec = ReferencedCoordinateSystemSpecification(
        CoordinateSystemType::geographic,
        "otherTestSystem",
        {}, {}, {});

    auto data = genPolyData<TransformedPolyData>();
    data->specifyCoordinateSystem(coordsSpec);
    auto transform = vtkSmartPointer<vtkTransform>::New();
    transform->Translate(3, 4, 5);
    data->setTransform(transform);

    const auto dataSetBounds = DataBounds(data->dataSet()->GetBounds());
    const auto dataBounds = data->bounds();
    const auto shiftedBounds = data->bounds().shifted(vtkVector3d(3, 4, 5));
    auto rendered = data->createRendered();
    rendered->setDefaultCoordinateSystem(targetCoordsSpec);
    const auto visibleBounds = rendered->visibleBounds();

    ASSERT_EQ(dataSetBounds, dataBounds);
    ASSERT_EQ(shiftedBounds, visibleBounds);
}
