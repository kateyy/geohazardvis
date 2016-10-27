#include <gtest/gtest.h>

#include <vtkDataSet.h>
#include <vtkCellData.h>
#include <vtkImageData.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTriangle.h>
#include <vtkVertex.h>

#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PointCloudDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/table_model/QVtkTableModel.h>
#include <core/data_objects/DataObject_private.h>
#include <core/utility/DataExtent.h>


class DataObject_test_DataObject : public DataObject
{
public:
    DataObject_test_DataObject() : DataObject("", nullptr) { }
    DataObject_test_DataObject(const QString & name, vtkDataSet * dataSet)
        : DataObject(name, dataSet)
    {
    }

    bool is3D() const override { return false; }

    const QString & dataTypeName() const override { static QString empty; return empty; }

    bool isDeferringEvents()
    {
        return dPtr().lockEventDeferrals().isDeferringEvents();
    }

protected:
    std::unique_ptr<QVtkTableModel> createTableModel() override { return nullptr; }
};

using TestDataObject = DataObject_test_DataObject;


class DataObject_test : public testing::Test
{
public:
    void SetUp() override
    {
    }
    void TearDown() override
    {
    }

    static vtkSmartPointer<vtkPolyData> createPolyDataPoints(vtkIdType numPoints)
    {
        auto polyData = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->SetNumberOfPoints(numPoints);
        for (vtkIdType i = 0; i < numPoints; ++i)
        {
            points->SetPoint(i,
                static_cast<double>(i + 1),
                static_cast<double>(numPoints - 1),
                static_cast<double>(numPoints));
        }
        polyData->SetPoints(points);
        return polyData;
    }
};

TEST_F(DataObject_test, ClearAttributesKeepsNameArray)
{
    const char * const nameArrayName = "Name";
    auto dataSet = vtkSmartPointer<vtkPolyData>::New();
    auto data = std::make_unique<TestDataObject>("noname", dataSet);
    ASSERT_TRUE(dataSet->GetFieldData()->HasArray(nameArrayName));

    data->clearAttributes();
    ASSERT_TRUE(dataSet->GetFieldData()->HasArray(nameArrayName));
}

TEST_F(DataObject_test, ClearAttributesClearsAdditionalAttributes)
{
    auto dataSet = vtkSmartPointer<vtkPolyData>::New();

    auto pointAttr = vtkSmartPointer<vtkFloatArray>::New();
    pointAttr->SetName("pointAttr");
    dataSet->GetPointData()->AddArray(pointAttr);
    auto cellAttr = vtkSmartPointer<vtkFloatArray>::New();
    cellAttr->SetName("cellAttr");
    dataSet->GetCellData()->AddArray(cellAttr);
    auto fieldAttr = vtkSmartPointer<vtkFloatArray>::New();
    fieldAttr->SetName("fieldAttr");
    dataSet->GetFieldData()->AddArray(fieldAttr);

    auto data = std::make_unique<TestDataObject>("noname", dataSet);

    ASSERT_TRUE(dataSet->GetPointData()->HasArray(pointAttr->GetName()));
    ASSERT_TRUE(dataSet->GetCellData()->HasArray(cellAttr->GetName()));
    ASSERT_TRUE(dataSet->GetFieldData()->HasArray(fieldAttr->GetName()));

    data->clearAttributes();

    ASSERT_FALSE(dataSet->GetPointData()->HasArray(pointAttr->GetName()));
    ASSERT_FALSE(dataSet->GetCellData()->HasArray(cellAttr->GetName()));
    ASSERT_FALSE(dataSet->GetFieldData()->HasArray(fieldAttr->GetName()));
}

class IntrinsicAttributesDataObject : public TestDataObject
{
public:
    IntrinsicAttributesDataObject(const QString & name, vtkDataSet * dataSet)
        : TestDataObject(name, dataSet)
    {
    }

protected:
    void addIntrinsicAttributes(
        QStringList & fieldAttributes,
        QStringList & pointAttributes,
        QStringList & cellAttributes) override
    {
        fieldAttributes << "intrFieldAttr";
        pointAttributes << "intrPointAttr";
        cellAttributes << "intrCellAttr";
    }
};

TEST_F(DataObject_test, ClearAttributesKeepsIntrinsicAttributes)
{
    auto dataSet = vtkSmartPointer<vtkPolyData>::New();

    auto pointAttr = vtkSmartPointer<vtkFloatArray>::New();
    pointAttr->SetName("pointAttr");
    dataSet->GetPointData()->AddArray(pointAttr);
    auto cellAttr = vtkSmartPointer<vtkFloatArray>::New();
    cellAttr->SetName("cellAttr");
    dataSet->GetCellData()->AddArray(cellAttr);
    auto fieldAttr = vtkSmartPointer<vtkFloatArray>::New();
    fieldAttr->SetName("fieldAttr");
    dataSet->GetFieldData()->AddArray(fieldAttr);

    auto intrPointAttr = vtkSmartPointer<vtkFloatArray>::New();
    intrPointAttr->SetName("intrPointAttr");
    dataSet->GetPointData()->AddArray(intrPointAttr);
    auto intrCellAttr = vtkSmartPointer<vtkFloatArray>::New();
    intrCellAttr->SetName("intrCellAttr");
    dataSet->GetCellData()->AddArray(intrCellAttr);
    auto intrFieldAttr = vtkSmartPointer<vtkFloatArray>::New();
    intrFieldAttr->SetName("intrFieldAttr");
    dataSet->GetFieldData()->AddArray(intrFieldAttr);

    auto data = std::make_unique<IntrinsicAttributesDataObject>("noname", dataSet);

    ASSERT_TRUE(dataSet->GetPointData()->HasArray(pointAttr->GetName()));
    ASSERT_TRUE(dataSet->GetCellData()->HasArray(cellAttr->GetName()));
    ASSERT_TRUE(dataSet->GetFieldData()->HasArray(fieldAttr->GetName()));
    ASSERT_TRUE(dataSet->GetPointData()->HasArray(intrPointAttr->GetName()));
    ASSERT_TRUE(dataSet->GetCellData()->HasArray(intrCellAttr->GetName()));
    ASSERT_TRUE(dataSet->GetFieldData()->HasArray(intrFieldAttr->GetName()));

    data->clearAttributes();

    ASSERT_FALSE(dataSet->GetPointData()->HasArray(pointAttr->GetName()));
    ASSERT_FALSE(dataSet->GetCellData()->HasArray(cellAttr->GetName()));
    ASSERT_FALSE(dataSet->GetFieldData()->HasArray(fieldAttr->GetName()));
    ASSERT_TRUE(dataSet->GetPointData()->HasArray(intrPointAttr->GetName()));
    ASSERT_TRUE(dataSet->GetCellData()->HasArray(intrCellAttr->GetName()));
    ASSERT_TRUE(dataSet->GetFieldData()->HasArray(intrFieldAttr->GetName()));
}

TEST_F(DataObject_test, CopyStructure_triggers_structureChanged_ImageDataObject)
{
    auto dataSet = vtkSmartPointer<vtkImageData>::New();

    auto referenceDataSet = vtkSmartPointer<vtkImageData>::New();
    referenceDataSet->SetExtent(1, 3, 1, 4, 0, 0);
    referenceDataSet->AllocateScalars(VTK_FLOAT, 1);

    bool structureChangedSignaled = false;

    ImageDataObject image("testImage", *dataSet);
    QObject::connect(&image, &DataObject::structureChanged,
        [&structureChangedSignaled] ()
    {
        structureChangedSignaled = true;
    });

    dataSet->CopyStructure(referenceDataSet);

    ASSERT_TRUE(structureChangedSignaled);
}

TEST_F(DataObject_test, CopyStructure_triggers_structureChanged_PolyDataObject)
{
    auto dataSet = vtkSmartPointer<vtkPolyData>::New();

    auto referenceDataSet = createPolyDataPoints(1);

    bool structureChangedSignaled = false;

    PolyDataObject poly("testPoly", *dataSet);
    QObject::connect(&poly, &DataObject::structureChanged,
        [&structureChangedSignaled] ()
    {
        structureChangedSignaled = true;
    });

    dataSet->CopyStructure(referenceDataSet);

    ASSERT_TRUE(structureChangedSignaled);
}

TEST_F(DataObject_test, CopyStructure_updates_ImageDataObject_extent)
{
    ImageExtent extent({ 1, 3, 1, 4, 0, 0 });

    auto dataSet = vtkSmartPointer<vtkImageData>::New();

    auto referenceDataSet = vtkSmartPointer<vtkImageData>::New();
    referenceDataSet->SetExtent(extent.data());
    referenceDataSet->AllocateScalars(VTK_FLOAT, 1);

    ImageDataObject image("testImage", *dataSet);

    dataSet->CopyStructure(referenceDataSet);

    ASSERT_EQ(extent, image.extent());
}

TEST_F(DataObject_test, CopyStructure_updates_VectorGrid3DDataObject_extent)
{
    ImageExtent extent({ 1, 3, 1, 4, 3, 5 });

    auto dataSet = vtkSmartPointer<vtkImageData>::New();

    auto referenceDataSet = vtkSmartPointer<vtkImageData>::New();
    referenceDataSet->SetExtent(extent.data());
    referenceDataSet->AllocateScalars(VTK_FLOAT, 1);

    VectorGrid3DDataObject grid("testGrid", *dataSet);

    dataSet->CopyStructure(referenceDataSet);

    ASSERT_EQ(extent, grid.extent());
}

TEST_F(DataObject_test, CopyStructure_Modified_updates_PolyDataObject_pointsCells)
{
    const vtkIdType numPoints = 3;
    const vtkIdType numCells = 2;

    auto dataSet = vtkSmartPointer<vtkPolyData>::New();

    auto referenceDataSet = createPolyDataPoints(3);
    auto cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(3, std::vector<vtkIdType>({ 0, 1, 2 }).data());
    cells->InsertNextCell(3, std::vector<vtkIdType>({ 0, 2, 1 }).data());
    referenceDataSet->SetVerts(cells);

    PolyDataObject poly("testPoly", *dataSet);

    dataSet->CopyStructure(referenceDataSet);
    dataSet->Modified();

    ASSERT_EQ(numPoints, poly.numberOfPoints());
    ASSERT_EQ(numCells, poly.numberOfCells());
}

TEST_F(DataObject_test, DataObject_CopyStructure_updates_PolyDataObject_pointsCells)
{
    const vtkIdType numPoints = 3;
    const vtkIdType numCells = 2;

    auto dataSet = vtkSmartPointer<vtkPolyData>::New();

    auto referenceDataSet = createPolyDataPoints(3);
    auto cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(3, std::vector<vtkIdType>({ 0, 1, 2 }).data());
    cells->InsertNextCell(3, std::vector<vtkIdType>({ 0, 2, 1 }).data());
    referenceDataSet->SetVerts(cells);

    PolyDataObject poly("testPoly", *dataSet);

    poly.CopyStructure(*referenceDataSet);

    ASSERT_EQ(numPoints, poly.numberOfPoints());
    ASSERT_EQ(numCells, poly.numberOfCells());
}

TEST_F(DataObject_test, GenericPolyDataObject_instantiate_PolyDataObject)
{
    auto polyData = createPolyDataPoints(3);
    auto triangles = vtkSmartPointer<vtkCellArray>::New();
    auto triangle = vtkSmartPointer<vtkTriangle>::New();
    triangle->GetPointIds()->SetId(0, 0);
    triangle->GetPointIds()->SetId(1, 1);
    triangle->GetPointIds()->SetId(2, 2);
    triangles->InsertNextCell(triangle);
    polyData->SetPolys(triangles);

    auto polyDataObject = GenericPolyDataObject::createInstance("triangles", *polyData);
    ASSERT_TRUE(polyDataObject);
    ASSERT_TRUE(dynamic_cast<PolyDataObject *>(polyDataObject.get()));
    ASSERT_EQ(polyData.Get(), polyDataObject->dataSet());
}

TEST_F(DataObject_test, GenericPolyDataObject_instantiate_PointCloudDataObject)
{
    auto polyData = createPolyDataPoints(1);
    auto vertices = vtkSmartPointer<vtkCellArray>::New();
    auto vertex = vtkSmartPointer<vtkVertex>::New();
    vertex->GetPointIds()->SetId(0, 0);
    vertices->InsertNextCell(vertex);
    polyData->SetVerts(vertices);

    auto pointCloudDataObject = GenericPolyDataObject::createInstance("vertices", *polyData);
    ASSERT_TRUE(pointCloudDataObject);
    ASSERT_TRUE(dynamic_cast<PointCloudDataObject *>(pointCloudDataObject.get()));
    ASSERT_EQ(polyData.Get(), pointCloudDataObject->dataSet());
}

TEST(ScopedEventDeferral_test, ExecutesDeferred)
{
    TestDataObject data("noname", vtkSmartPointer<vtkPolyData>::New());

    bool isSignaled = false;

    QObject::connect(&data, &DataObject::attributeArraysChanged, [&isSignaled] () -> void
    {
        isSignaled = true;
    });

    {
        ScopedEventDeferral d0(data);

        data.dataSet()->GetPointData()->Modified();

        ASSERT_FALSE(isSignaled);
    }

    ASSERT_TRUE(isSignaled);
}

TEST(ScopedEventDeferral_test, move_lock)
{
    TestDataObject data;

    {
        ASSERT_FALSE(data.isDeferringEvents());

        ScopedEventDeferral d0(data);

        ASSERT_TRUE(data.isDeferringEvents());

        auto moveDeferral = [&data]() -> ScopedEventDeferral
        {
            ScopedEventDeferral d(data);

            return std::move(d);
        };

        auto d2 = moveDeferral();

        ASSERT_TRUE(data.isDeferringEvents());
    }

    ASSERT_FALSE(data.isDeferringEvents());
}
