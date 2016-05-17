#pragma once

#include <functional>
#include <mutex>

#include <QList>
#include <QObject>

#include <core/core_api.h>


class DataObject;
class DataSetHandler;


/** Utility class that provides a filtered view to data sets hold by a DataSetHandler.
    listChanged is emitted whenever the DataSetHandler's lists change in a way that the list
    of items filtered by the filter function (setFilter) changes.
    It is necessary to set a filter function to use this class.
*/
class CORE_API DataSetFilter : public QObject
{
    Q_OBJECT

public:
    DataSetFilter(const DataSetHandler & dataSetHandler, QObject * parent = nullptr);
    ~DataSetFilter() override;

    using t_filterFunction = std::function<bool(DataObject * dataSet, const DataSetHandler & dataSetHandler)>;

    /** Set a filter function that identifies relevant data sets from the data sets hold by the current DataSetHandler.
      * This is null by default and doesn't pass any data sets. */
    void setFilterFunction(const t_filterFunction & filterFunction);
    template<typename T> void setFilterFunction(const T & filterFunctor);

    const QList<DataObject *> & filteredDataSetList() const;

    /** Calls blockUpdates on construction and, if release was not called, discardUpdates on deletion.
      * release redirects to releaseUpdates. If release is called, the destructor doesn't do anything. */
    class CORE_API ScopedLock
    {
    public:
        explicit ScopedLock(DataSetFilter & dataSetFilter);
        ~ScopedLock();

        void release();

        ScopedLock(ScopedLock && other);
        ScopedLock(const ScopedLock &) = delete;
        void operator=(const ScopedLock &) = delete;

    private:
        DataSetFilter * m_dataSetFilter;
        std::unique_lock<std::recursive_mutex> m_lock;
    };

    /** Create a scoped lock that is meant to be used according to the RAII pattern. */
    ScopedLock scopedLock();

signals:
    void listChanged(const QList<DataObject *> & filteredList);

private:
    void lockAndUpdateList();
    void updateList(std::unique_lock<std::recursive_mutex> && lock);

    /** Finish currently running updates and block further updates until discardUpdates or releaseUpdates are called. */
    std::unique_lock<std::recursive_mutex> blockUpdates();
    /** Discard previously blocked updates and unset the filter function, also discarding any further updates. */
    void discardUpdates(std::unique_lock<std::recursive_mutex> && lock);
    /** Execute previously blocked updates and handle further updates as before calling blockUpdates. */
    void releaseUpdates(std::unique_lock<std::recursive_mutex> && lock);

private:
    const DataSetHandler & m_dataSetHandler;

    mutable std::recursive_mutex m_mutex;
    t_filterFunction m_filterFunction;
    QList<DataObject *> m_filteredDataSetList;
    int m_deferUpdatesRequests;
};


template<typename T>
void DataSetFilter::setFilterFunction(const T & filterFunctor)
{
    setFilterFunction(t_filterFunction(filterFunctor));
}
