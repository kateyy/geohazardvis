#include <gtest/gtest.h>

#include <vtkDataSet.h>
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <core/data_objects/DataObject.h>
#include <core/table_model/QVtkTableModel.h>
#include <core/data_objects/DataObject_private.h>


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
