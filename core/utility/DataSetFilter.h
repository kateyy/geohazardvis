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

    /** Calls blockUpdates on construction and, by default, releaseUpdates on deletion.
      * If discardFurtherUpdates is called, the filter function be unset, thus blocking all further
      * update calls. */
    class CORE_API ScopedLock
    {
    public:
        explicit ScopedLock(DataSetFilter & dataSetFilter);
        ~ScopedLock();

        void discardFurtherUpdates();

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
