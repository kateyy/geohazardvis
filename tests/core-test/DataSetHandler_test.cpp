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

#include <memory>

#include <vtkPolyData.h>
#include <vtkFloatArray.h>

#include <core/DataSetHandler.h>
#include <core/types.h>
#include <core/data_objects/DataObject.h>
#include <core/data_objects/RawVectorData.h>
#include <core/table_model/QVtkTableModel.h>


class DataSetHandler_test_DataObject : public DataObject
{
public:
    explicit DataSetHandler_test_DataObject(bool * destructorCalledFlag = nullptr)
        : DataObject("test data", vtkSmartPointer<vtkPolyData>::New())
        , m_desturctorCalledFlag(destructorCalledFlag)
    {
        if (destructorCalledFlag)
        {
            *destructorCalledFlag = false;
        }
    }
    ~DataSetHandler_test_DataObject() override
    {
        if (m_desturctorCalledFlag)
        {
            *m_desturctorCalledFlag = true;
        }
    }

    std::unique_ptr<DataObject> newInstance(const QString & /*name*/, vtkDataSet * /*dataSet*/) const override
    {
        return{};
    }

    bool is3D() const override { return false; }
    IndexType defaultAttributeLocation() const override { return IndexType::points; }
    const QString & dataTypeName() const override { return m_dataTypeName; }
    std::unique_ptr<QVtkTableModel> createTableModel() override { return nullptr; }

private:
    const QString m_dataTypeName = "nothing specific";
    bool * m_desturctorCalledFlag = nullptr;
};

using TestDataObject = DataSetHandler_test_DataObject;


class TestRawVector : public RawVectorData
{
public:
    TestRawVector(vtkFloatArray & data, bool * destructorCalledFlag = nullptr)
        : RawVectorData("test vector", data)
        , m_desturctorCalledFlag(destructorCalledFlag)
    {
        if (destructorCalledFlag)
        {
            *destructorCalledFlag = false;
        }
    }
    ~TestRawVector() override
    {
        if (m_desturctorCalledFlag)
        {
            *m_desturctorCalledFlag = true;
        }
    }

private:
    bool * m_desturctorCalledFlag = nullptr;
};

class DataSetHandler_test: public ::testing::Test
{
public:
    void SetUp() override
    {
        handler = std::make_unique<DataSetHandler>();
    }
    void TearDown() override
    {
        handler.reset();
    }

    std::unique_ptr<DataSetHandler> handler;
};

TEST_F(DataSetHandler_test, AddExternalDataSet)
{
    std::unique_ptr<DataObject> dataObject = std::make_unique<TestDataObject>();
    auto dataObjectPtr = dataObject.get();

    handler->addExternalData({ dataObjectPtr });

    ASSERT_TRUE(handler->dataSets().contains(dataObjectPtr));
    ASSERT_TRUE(handler->dataSetOwnerships().contains(dataObjectPtr));
    ASSERT_FALSE(handler->dataSetOwnerships().value(dataObjectPtr));
}

TEST_F(DataSetHandler_test, AddExternalRawVector)
{
    auto rawDataArray = vtkSmartPointer<vtkFloatArray>::New();
    std::unique_ptr<DataObject> rawVector = std::make_unique<RawVectorData>("", *rawDataArray);
    auto rawVectorPtr = static_cast<RawVectorData *>(rawVector.get());

    handler->addExternalData({ rawVectorPtr });

    ASSERT_FALSE(handler->dataSets().contains(rawVectorPtr));
    ASSERT_FALSE(handler->dataSetOwnerships().contains(rawVectorPtr));

    ASSERT_TRUE(handler->rawVectors().contains(rawVectorPtr));
    ASSERT_TRUE(handler->rawVectorOwnerships().contains(rawVectorPtr));
    ASSERT_FALSE(handler->rawVectorOwnerships().value(rawVectorPtr));
}

TEST_F(DataSetHandler_test, TakeDataSet)
{
    std::unique_ptr<DataObject> dataObject = std::make_unique<TestDataObject>();
    auto dataObjectPtr = dataObject.get();

    handler->takeData(std::move(dataObject));

    ASSERT_EQ(dataObject, nullptr);
    ASSERT_TRUE(handler->dataSets().contains(dataObjectPtr));
    ASSERT_TRUE(handler->dataSetOwnerships().contains(dataObjectPtr));
    ASSERT_TRUE(handler->dataSetOwnerships().value(dataObjectPtr));
}

TEST_F(DataSetHandler_test, TakeRawVector)
{
    auto rawDataArray = vtkSmartPointer<vtkFloatArray>::New();
    std::unique_ptr<DataObject> rawVector = std::make_unique<RawVectorData>("", *rawDataArray);
    auto rawVectorPtr = static_cast<RawVectorData *>(rawVector.get());

    handler->takeData(std::move(rawVector));

    ASSERT_EQ(rawVector, nullptr);
    ASSERT_FALSE(handler->dataSets().contains(rawVectorPtr));
    ASSERT_FALSE(handler->dataSetOwnerships().contains(rawVectorPtr));

    ASSERT_TRUE(handler->rawVectors().contains(rawVectorPtr));
    ASSERT_TRUE(handler->rawVectorOwnerships().contains(rawVectorPtr));
    ASSERT_TRUE(handler->rawVectorOwnerships().value(rawVectorPtr));
}

TEST_F(DataSetHandler_test, RemoveExternalDataSet)
{
    bool deleted;

    std::unique_ptr<DataObject> dataObject = std::make_unique<TestDataObject>(&deleted);
    auto dataObjectPtr = dataObject.get();

    handler->addExternalData({ dataObjectPtr });
    handler->removeExternalData({ dataObjectPtr });

    ASSERT_FALSE(handler->dataSets().contains(dataObjectPtr));
    ASSERT_FALSE(handler->dataSetOwnerships().contains(dataObjectPtr));
    ASSERT_FALSE(deleted);

    handler->addExternalData({ dataObjectPtr });
    handler->deleteData({ dataObjectPtr });

    ASSERT_TRUE(handler->dataSets().contains(dataObjectPtr));
    ASSERT_TRUE(handler->dataSetOwnerships().contains(dataObjectPtr));
    ASSERT_FALSE(handler->dataSetOwnerships().value(dataObjectPtr));
    ASSERT_FALSE(deleted);
}

TEST_F(DataSetHandler_test, RemoveExternalRawVector)
{
    bool deleted;

    auto rawDataArray = vtkSmartPointer<vtkFloatArray>::New();
    std::unique_ptr<DataObject> rawVector = std::make_unique<TestRawVector>(*rawDataArray, &deleted);
    auto rawVectorPtr = static_cast<RawVectorData *>(rawVector.get());

    handler->addExternalData({ rawVectorPtr });
    handler->removeExternalData({ rawVectorPtr });

    ASSERT_FALSE(handler->rawVectors().contains(rawVectorPtr));
    ASSERT_FALSE(handler->rawVectorOwnerships().contains(rawVectorPtr));
    ASSERT_FALSE(deleted);

    handler->addExternalData({ rawVectorPtr });
    handler->deleteData({ rawVectorPtr });

    ASSERT_TRUE(handler->rawVectors().contains(rawVectorPtr));
    ASSERT_TRUE(handler->rawVectorOwnerships().contains(rawVectorPtr));
    ASSERT_FALSE(handler->rawVectorOwnerships().value(rawVectorPtr));
    ASSERT_FALSE(deleted);
}

TEST_F(DataSetHandler_test, DeleteOwnedDataSet)
{
    bool deleted;

    std::unique_ptr<DataObject> dataObject = std::make_unique<TestDataObject>(&deleted);
    auto dataObjectPtr = dataObject.get();

    handler->takeData(std::move(dataObject));
    handler->deleteData({ dataObjectPtr });

    ASSERT_FALSE(handler->dataSets().contains(dataObjectPtr));
    ASSERT_FALSE(handler->dataSetOwnerships().contains(dataObjectPtr));
    ASSERT_TRUE(deleted);
}

TEST_F(DataSetHandler_test, DeletedOwnedRawVector)
{
    bool deleted;

    auto rawDataArray = vtkSmartPointer<vtkFloatArray>::New();
    std::unique_ptr<DataObject> rawVector = std::make_unique<TestRawVector>(*rawDataArray, &deleted);
    auto rawVectorPtr = static_cast<RawVectorData *>(rawVector.get());

    handler->takeData(std::move(rawVector));
    handler->deleteData({ rawVectorPtr });

    ASSERT_FALSE(handler->rawVectors().contains(rawVectorPtr));
    ASSERT_FALSE(handler->rawVectorOwnerships().contains(rawVectorPtr));
    ASSERT_TRUE(deleted);
}
