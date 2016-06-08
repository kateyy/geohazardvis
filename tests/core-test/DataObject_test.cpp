#include <gtest/gtest.h>

#include <core/data_objects/DataObject.h>
#include <core/table_model/QVtkTableModel.h>
#include <core/data_objects/DataObject_private.h>


class DataObject_test_DataObject : public DataObject
{
public:
    DataObject_test_DataObject() : DataObject("", nullptr) { }

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
