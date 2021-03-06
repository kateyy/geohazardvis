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

#include <QSet>

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include <core/DataSetHandler.h>
#include <core/types.h>
#include <core/data_objects/DataObject.h>
#include <core/table_model/QVtkTableModel.h>
#include <core/utility/DataSetFilter.h>


class TestDataObject : public DataObject
{
public:
    TestDataObject(const QString & name = "noname")
        : DataObject(name, vtkSmartPointer<vtkImageData>::New())
    {
    }

    std::unique_ptr<DataObject> newInstance(const QString & name, vtkDataSet * /*dataSet*/) const override
    {
        return std::make_unique<TestDataObject>(name);
    }

    bool is3D() const override
    {
        return false;
    }

    IndexType defaultAttributeLocation() const override
    {
        return IndexType::points;
    }

    const QString & dataTypeName() const override
    {
        static const QString name = "testType";
        return name;
    }

    std::unique_ptr<QVtkTableModel> createTableModel() override
    {
        return{};
    }
};

class DataSetFilter_test : public ::testing::Test
{
public:
    void SetUp() override
    {
        env.dataSetHandler = std::make_unique<DataSetHandler>();
    }

    struct TestEnv
    {
        std::unique_ptr<DataSetHandler> dataSetHandler;
    };

    static TestEnv env;

    class Test_DataSetFilter : public DataSetFilter
    {
    public:
        Test_DataSetFilter()
            : DataSetFilter(*env.dataSetHandler)
        {

        }
    };
};


DataSetFilter_test::TestEnv DataSetFilter_test::env;


TEST_F(DataSetFilter_test, ListsCorrectObjects)
{
    Test_DataSetFilter filter;

    filter.setFilterFunction([] (DataObject * dataSet, const DataSetHandler &) -> bool
    {
        auto s = dataSet->name().toLower();
        s.truncate(2);
        return s == "ok";
    });
    
    TestDataObject a("ok a");
    TestDataObject b("no b");

    env.dataSetHandler->addExternalData({ &a, &b });
    ASSERT_EQ(QSet<DataObject *>{&a}, filter.filteredDataSetList().toSet());
}

TEST_F(DataSetFilter_test, SendsSignalOnChange)
{
    Test_DataSetFilter filter;

    filter.setFilterFunction([] (DataObject *, const DataSetHandler &) -> bool
    {
        return true;
    });

    bool changed = false;

    QObject::connect(&filter, &DataSetFilter::listChanged, [&changed] () 
    {
        changed = true;
    });
    
    TestDataObject a;
    env.dataSetHandler->addExternalData({ &a });
    ASSERT_TRUE(changed);
}

TEST_F(DataSetFilter_test, DiscardsUpdatesWhileLocked)
{
    Test_DataSetFilter filter;

    filter.setFilterFunction([] (DataObject *, const DataSetHandler &) -> bool
    {
        return true;
    });

    bool changed = false;
    
    TestDataObject a("a");
    TestDataObject b("b");

    env.dataSetHandler->addExternalData({ &a });

    auto lock = filter.scopedLock();

    QObject::connect(&filter, &DataSetFilter::listChanged, [&changed] ()
    {
        changed = true;
    });

    env.dataSetHandler->addExternalData({ &b });

    ASSERT_FALSE(changed);
    ASSERT_EQ(QSet<DataObject *>{&a}, filter.filteredDataSetList().toSet());
}

TEST_F(DataSetFilter_test, RegistersChangesAfterUnlock)
{
    Test_DataSetFilter filter;

    filter.setFilterFunction([] (DataObject *, const DataSetHandler &) -> bool
    {
        return true;
    });

    bool changed = false;

    TestDataObject a("a");
    TestDataObject b("b");
    TestDataObject c("c");

    env.dataSetHandler->addExternalData({ &a });

    QObject::connect(&filter, &DataSetFilter::listChanged, [&changed] ()
    {
        changed = true;
    });

    {
        auto lock = filter.scopedLock();

        env.dataSetHandler->addExternalData({ &b });
    }

    env.dataSetHandler->addExternalData({ &c });

    ASSERT_TRUE(changed);
    ASSERT_EQ(QSet<DataObject *>({ &a, &b, &c }), filter.filteredDataSetList().toSet());
}

TEST_F(DataSetFilter_test, FiltersCorrectlyAfterUnlock)
{
    Test_DataSetFilter filter;

    filter.setFilterFunction([] (DataObject * dataSet, const DataSetHandler &) -> bool
    {
        auto s = dataSet->name().toLower();
        s.truncate(2);
        return s == "ok";
    });

    TestDataObject a("ok a");
    TestDataObject b("no b");

    {
        auto lock = filter.scopedLock();
    }

    env.dataSetHandler->addExternalData({ &a, &b });

    ASSERT_EQ(QSet<DataObject *>({ &a }), filter.filteredDataSetList().toSet());
}

TEST_F(DataSetFilter_test, TriggersSignalAfterRelease)
{
    Test_DataSetFilter filter;

    filter.setFilterFunction([] (DataObject *, const DataSetHandler &) -> bool
    {
        return true;
    });

    bool changed = false;

    TestDataObject a("a");
    TestDataObject b("b");

    env.dataSetHandler->addExternalData({ &a });

    QObject::connect(&filter, &DataSetFilter::listChanged, [&changed] ()
    {
        changed = true;
    });

    {
        auto lock = filter.scopedLock();

        env.dataSetHandler->addExternalData({ &b });
    }

    ASSERT_TRUE(changed);
    ASSERT_EQ(QSet<DataObject *>({ &a, &b }), filter.filteredDataSetList().toSet());
}
