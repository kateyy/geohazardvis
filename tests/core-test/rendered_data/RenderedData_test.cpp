#include <gtest/gtest.h>

#include <vtkActor.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkMapper.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPropCollection.h>

#include <core/CoordinateSystems.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>


class RenderedData_test : public ::testing::Test
{
public:
    void SetUp() override
    {
    }
    void TearDown() override
    {
    }

    static std::unique_ptr<GenericPolyDataObject> genPolyData(const DataBounds & bounds, int cellType)
    {
        auto poly = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->SetDataTypeToDouble();
        points->InsertNextPoint(bounds.min().GetData());
        points->InsertNextPoint(bounds.min()[0], bounds.max()[1], bounds.min()[2]);
        points->InsertNextPoint(bounds.max().GetData());
        poly->SetPoints(points);
        std::array<vtkIdType, 3> pointIds = { 0, 1, 2 };
        poly->Allocate(static_cast<vtkIdType>(pointIds.size()));
        poly->InsertNextCell(cellType, static_cast<int>(pointIds.size()), pointIds.data());
        assert(DataBounds(poly->GetBounds()) == bounds);

        return GenericPolyDataObject::createInstance("PolyData", *poly);
    }

    template<typename T, int Dims>
    static std::unique_ptr<T> genImageData(const DataBounds & bounds)
    {
        auto image = vtkSmartPointer<vtkImageData>::New();
        image->SetExtent(0, 3, 0, 3, 0, Dims == 3 ? 3 : 0);
        image->SetOrigin(bounds.min().GetData());
        image->SetSpacing(convertTo<3>(convertTo<Dims>(bounds.componentSize()) / 3.0, 1.0).GetData());
        assert(DataBounds(image->GetBounds()) == bounds);

        auto scalars = vtkSmartPointer<vtkFloatArray>::New();
        scalars->SetNumberOfComponents(Dims == 3 ? 3 : 1);
        scalars->SetNumberOfTuples(image->GetNumberOfPoints());
        scalars->SetName("scalars");
        image->GetPointData()->SetScalars(scalars);

        return std::make_unique<T>("GridData", *image);
    }
};

TEST_F(RenderedData_test, RenderedPolyData_SetsMapperInfo)
{
    auto data = vtkSmartPointer<vtkPolyData>::New();

    auto dataObject = std::make_unique<PolyDataObject>("noname", *data);

    auto rendered = dataObject->createRendered();

    DataObject * dataObjectPtr = dataObject.get();
    AbstractVisualizedData * visDataPtr = rendered.get();

    auto && viewProps = rendered->viewProps();
    for (viewProps->InitTraversal(); auto actor = vtkActor::SafeDownCast(viewProps->GetNextProp());)
    {
        auto mapper = actor->GetMapper();
        if (!mapper)
        {
            continue;
        }

        auto & mapperInfo = *mapper->GetInformation();

        ASSERT_EQ(DataObject::readPointer(mapperInfo), dataObjectPtr);
        ASSERT_EQ(AbstractVisualizedData::readPointer(mapperInfo), visDataPtr);
    }
}

TEST_F(RenderedData_test, ImageData_SetsMapperInfo)
{
    auto data = vtkSmartPointer<vtkImageData>::New();

    auto dataObject = std::make_unique<ImageDataObject>("noname", *data);

    auto rendered = dataObject->createRendered();

    DataObject * dataObjectPtr = dataObject.get();
    AbstractVisualizedData * visDataPtr = rendered.get();

    auto && viewProps = rendered->viewProps();
    for (viewProps->InitTraversal(); auto actor = vtkActor::SafeDownCast(viewProps->GetNextProp());)
    {
        auto mapper = actor->GetMapper();
        if (!mapper)
        {
            continue;
        }

        auto & mapperInfo = *mapper->GetInformation();

        ASSERT_EQ(DataObject::readPointer(mapperInfo), dataObjectPtr);
        ASSERT_EQ(AbstractVisualizedData::readPointer(mapperInfo), visDataPtr);
    }
}

TEST_F(RenderedData_test, VectorGrid3D_SetsMapperInfo)
{
    auto data = vtkSmartPointer<vtkImageData>::New();
    data->SetDimensions(10, 10, 10);
    data->AllocateScalars(VTK_FLOAT, 3);
    data->GetPointData()->GetScalars()->SetName("nononame");

    auto dataObject = std::make_unique<VectorGrid3DDataObject>("noname", *data);

    auto rendered = dataObject->createRendered();

    DataObject * dataObjectPtr = dataObject.get();
    AbstractVisualizedData * visDataPtr = rendered.get();

    auto && viewProps = rendered->viewProps();
    for (viewProps->InitTraversal(); auto actor = vtkActor::SafeDownCast(viewProps->GetNextProp());)
    {
        auto mapper = actor->GetMapper();
        if (!mapper)
        {
            continue;
        }

        auto & mapperInfo = *mapper->GetInformation();

        ASSERT_EQ(DataObject::readPointer(mapperInfo), dataObjectPtr);
        ASSERT_EQ(AbstractVisualizedData::readPointer(mapperInfo), visDataPtr);
    }
}

TEST_F(RenderedData_test, RenderedPolyData_reportsVisibleBounds)
{
    const auto dataBounds = DataBounds({ -2, 5, -3, 6, -4, 7 });
    const auto visibleBounds = genPolyData(dataBounds, VTK_TRIANGLE)->createRendered()->visibleBounds();

    ASSERT_EQ(dataBounds, visibleBounds);
}

TEST_F(RenderedData_test, RenderedPointCloudData_reportsVisibleBounds)
{
    const auto dataBounds = DataBounds({ -2, 5, -3, 6, -4, 7 });
    const auto visibleBounds = genPolyData(dataBounds, VTK_VERTEX)->createRendered()->visibleBounds();

    ASSERT_EQ(dataBounds, visibleBounds);
}

TEST_F(RenderedData_test, RenderedImageData_reportsVisibleBounds)
{
    const auto dataBounds = DataBounds({ -2, 5, -3, 6, -4, -4 });
    const auto visibleBounds = genImageData<ImageDataObject, 2>(dataBounds)->createRendered()->visibleBounds();

    ASSERT_EQ(dataBounds, visibleBounds);
}

TEST_F(RenderedData_test, RenderedVectorGrid3D_reportsVisibleBounds)
{
    const auto dataBounds = DataBounds({ -2, 5, -3, 6, -4, 7 });
    const auto visibleBounds = genImageData<VectorGrid3DDataObject, 3>(dataBounds)->createRendered()->visibleBounds();

    ASSERT_EQ(dataBounds, visibleBounds);
}

TEST_F(RenderedData_test, RenderedPointCloudData_defaultColorMappingAndCoordinates)
{
    auto pointCloud = genPolyData(DataBounds({
        210590.35, 211590.35, // 1km x 1km
        3322575.90, 3323575.90,
        1, 1 }), VTK_VERTEX);
    const auto dataCoordsSpec = ReferencedCoordinateSystemSpecification(
        CoordinateSystemType::metricGlobal, "WGS 84", "UTM", "m",
        { 30.0, 60.0 });
    pointCloud->specifyCoordinateSystem(dataCoordsSpec);

    const auto numPoints = pointCloud->numberOfPoints();
    auto scalars = vtkSmartPointer<vtkFloatArray>::New();
    scalars->SetNumberOfValues(numPoints);
    scalars->SetName("PointScalars");
    pointCloud->polyDataSet().GetPointData()->SetScalars(scalars);

    auto rendered = pointCloud->createRendered();

    auto targetCoordsSpec = dataCoordsSpec;
    targetCoordsSpec.type = CoordinateSystemType::metricLocal;
    targetCoordsSpec.unitOfMeasurement = "km";

    rendered->setDefaultCoordinateSystem(targetCoordsSpec);
    auto viewProps = rendered->viewProps();
    DataBounds renderedLocalKmBounds;
    for (viewProps->InitTraversal(); auto prop = viewProps->GetNextProp();)
    {
        renderedLocalKmBounds.add(DataBounds(prop->GetBounds()));
    }


    ASSERT_TRUE(rendered->colorMapping().isEnabled());
    ASSERT_STREQ(scalars->GetName(), rendered->colorMapping().currentScalarsName().toUtf8().data());

    const auto expectedBounds = DataBounds({ 0.0, 1.0, 0.0, 1.0, 0.001, 0.001 });
    for (size_t i = 0; i < 6; ++i)
    {
        ASSERT_NEAR(static_cast<float>(expectedBounds[i]), renderedLocalKmBounds[i],
            0.01); // max delta = 1cm
    }
}
