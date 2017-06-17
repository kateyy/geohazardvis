#include <gtest/gtest.h>

#include <QDir>

#include <vtkAlgorithmOutput.h>
#include <vtkImageData.h>
#include <vtkFieldData.h>
#include <vtkPolyData.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <core/CoordinateSystems.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/io/Exporter.h>
#include <core/io/Loader.h>
#include <core/rendered_data/RenderedData.h>
#include <core/utility/DataExtent.h>

#include "TestEnvironment.h"


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


class CoordinateTransformableDataObject_test : public ::testing::Test
{
public:
    void SetUp() override
    {
        TestEnvironment::createTestDir();
    }
    void TearDown() override
    {
        TestEnvironment::clearTestDir();
    }

    static vtkSmartPointer<vtkPolyData> genPolyDataSet()
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

        return poly;
    }

    template<typename T = PolyDataObject>
    static std::unique_ptr<T> genPolyData()
    {
        return std::make_unique<T>("PolyData", *genPolyDataSet());
    }
};

TEST(ReferencedCoordinateSystemSpecification_test, EqualIfBothInspecified)
{
    ReferencedCoordinateSystemSpecification spec1;
    spec1.type = CoordinateSystemType::geographic;
    spec1.geographicSystem = "geoSystem";
    spec1.globalMetricSystem = "metricSystem";
    
    auto spec2 = spec1;
    ASSERT_EQ(spec1, spec2);

    spec2.type = CoordinateSystemType::metricLocal;
    ASSERT_NE(spec1, spec2);

    spec1.referencePointLatLong = { 1.0, 2.0 };
    spec2 = spec1;
    ASSERT_EQ(spec1, spec2);
    spec2.referencePointLatLong = { 2.0, 3.0 };
    ASSERT_NE(spec1, spec2);
}

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

TEST_F(CoordinateTransformableDataObject_test, ApplyCoordinateSystemFromFieldData)
{
    auto dataSet = genPolyDataSet();

    const auto coordsSpec = ReferencedCoordinateSystemSpecification(
        CoordinateSystemType::geographic,
        "testGeoSystem",
        "testMetricSystem",
        "",
        { 100, 200 });

    coordsSpec.writeToFieldData(*dataSet->GetFieldData());
    
    PolyDataObject polyData("poly", *dataSet);

    ASSERT_EQ(coordsSpec, polyData.coordinateSystem());
}

TEST_F(CoordinateTransformableDataObject_test, PersistentCoordsSpec_PolyData)
{
    auto dataObject = genPolyData();

    const auto coordsSpec = ReferencedCoordinateSystemSpecification(
        CoordinateSystemType::geographic,
        "testGeoSystem",
        "testMetricSystem",
        "",
        { 100, 200 });

    dataObject->specifyCoordinateSystem(coordsSpec);

    const auto fileName = QDir(TestEnvironment::testDirPath()).filePath("PersistentCoordsSpec.vtp");

    ASSERT_TRUE(Exporter::exportData(*dataObject, fileName));
    auto readDataObject = Loader::readFile<CoordinateTransformableDataObject>(fileName);
    ASSERT_TRUE(readDataObject);

    ASSERT_EQ(coordsSpec, readDataObject->coordinateSystem());
}

TEST_F(CoordinateTransformableDataObject_test, PersistentCoordsSpec_ImageData)
{
    auto imageDataSet = vtkSmartPointer<vtkImageData>::New();
    imageDataSet->SetExtent(0, 1, 2, 4, 0, 0);
    imageDataSet->AllocateScalars(VTK_FLOAT, 1);
    
    ImageDataObject dataObject("image", *imageDataSet);

    const auto coordsSpec = ReferencedCoordinateSystemSpecification(
        CoordinateSystemType::geographic,
        "testGeoSystem",
        "testMetricSystem",
        "",
        { 100, 200 });

    dataObject.specifyCoordinateSystem(coordsSpec);

    const auto fileName = QDir(TestEnvironment::testDirPath()).filePath("PersistentCoordsSpec.vti");

    ASSERT_TRUE(Exporter::exportData(dataObject, fileName));
    auto readDataObject = Loader::readFile<CoordinateTransformableDataObject>(fileName);
    ASSERT_TRUE(readDataObject);

    ASSERT_EQ(coordsSpec, readDataObject->coordinateSystem());
}

TEST_F(CoordinateTransformableDataObject_test, coordsPassToProcessedFieldData)
{
    auto dataObject = genPolyData();

    const auto coordsSpec = ReferencedCoordinateSystemSpecification(
        CoordinateSystemType::geographic,
        "testGeoSystem",
        "testMetricSystem",
        "",
        { 100, 200 });

    dataObject->specifyCoordinateSystem(coordsSpec);

    auto processedDataSet = dataObject->processedOutputDataSet();
    ASSERT_TRUE(processedDataSet);
    const auto passedSpec = ReferencedCoordinateSystemSpecification::fromFieldData(*processedDataSet->GetFieldData());

    ASSERT_EQ(coordsSpec, passedSpec);
}
