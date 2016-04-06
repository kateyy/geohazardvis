#include "DataSetFilter.h"

#include <cassert>

#include <QSet>

#include <core/DataSetHandler.h>


DataSetFilter::DataSetFilter(const DataSetHandler & dataSetHandler, QObject * parent)
    : QObject(parent)
    , m_dataSetHandler{ dataSetHandler }
    , m_deferUpdatesRequests{ 0 }
{
    connect(&m_dataSetHandler, &DataSetHandler::dataObjectsChanged, this, &DataSetFilter::lockAndUpdateList);
}

DataSetFilter::~DataSetFilter() = default;

void DataSetFilter::setFilterFunction(const t_filterFunction & filterFunction)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    m_filterFunction = filterFunction;

    updateList(std::move(lock));
}

const QList<DataObject*> & DataSetFilter::filteredDataSetList() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    return m_filteredDataSetList;
}

DataSetFilter::ScopedLock::ScopedLock(DataSetFilter & dataSetFilter)
    : m_dataSetFilter{ &dataSetFilter }
    , m_lock{ m_dataSetFilter->blockUpdates() }
{
}

DataSetFilter::ScopedLock::ScopedLock(ScopedLock && other)
    : m_dataSetFilter{ other.m_dataSetFilter }
    , m_lock{ std::move(other.m_lock) }
{
    other.m_dataSetFilter = nullptr;
}

DataSetFilter::ScopedLock::~ScopedLock()
{
    if (m_dataSetFilter)
    {
        m_dataSetFilter->discardUpdates(std::move(m_lock));
    }
}

void DataSetFilter::ScopedLock::release()
{
    if (m_dataSetFilter)
    {
        m_dataSetFilter->releaseUpdates(std::move(m_lock));
    }

    m_dataSetFilter = nullptr;
}

DataSetFilter::ScopedLock DataSetFilter::scopedLock()
{
    return ScopedLock(*this);
}

void DataSetFilter::lockAndUpdateList()
{
    updateList(std::unique_lock<std::recursive_mutex>(m_mutex));
}

void DataSetFilter::updateList(std::unique_lock<std::recursive_mutex> && lock)
{
    lock;   // keep locked until out of scope

    if (m_deferUpdatesRequests > 0)
    {
        return;
    }

    if (!m_filterFunction)
    {
        return;
    }

    auto && dataSets = m_dataSetHandler.dataSets();

    QList<DataObject *> newFilteredList;

    for (auto dataSet : dataSets)
    {
        if (m_filterFunction(dataSet, m_dataSetHandler))
        {
            newFilteredList << dataSet;
        }
    }

    if (m_filteredDataSetList.toSet() == newFilteredList.toSet())
    {
        return;
    }

    m_filteredDataSetList = newFilteredList;

    emit listChanged(m_filteredDataSetList);
}

std::unique_lock<std::recursive_mutex> DataSetFilter::blockUpdates()
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    m_deferUpdatesRequests += 1;

    return lock;
}

void DataSetFilter::discardUpdates(std::unique_lock<std::recursive_mutex> && lock)
{
    m_filterFunction = nullptr;

    assert(m_deferUpdatesRequests > 0);
    m_deferUpdatesRequests -= 1;

    lock.unlock();
}

void DataSetFilter::releaseUpdates(std::unique_lock<std::recursive_mutex> && lock)
{
    assert(m_deferUpdatesRequests > 0);
    m_deferUpdatesRequests -= 1;

    updateList(std::move(lock));
}
