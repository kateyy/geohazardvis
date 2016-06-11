#include "DataObject_private.h"

#include <cassert>

#include <vtkDataSet.h>
#include <vtkInformationIntegerPointerKey.h>
#include <vtkInformationStringKey.h>
#include <vtkTrivialProducer.h>


vtkInformationKeyMacro(DataObjectPrivate, DataObjectKey, IntegerPointer);
vtkInformationKeyMacro(DataObjectPrivate, NameKey, String);


DataObjectPrivate::DataObjectPrivate(DataObject & dataObject, const QString & name, vtkDataSet * dataSet)
    : m_name{ name }
    , m_dataSet{ dataSet }
    , m_tableModel{}
    , m_bounds{}
    , m_numberOfPoints{ 0 }
    , m_numberOfCells{ 0 }
    , q_ptr{ dataObject }
    , m_deferEventsRequests{ 0 }
{
    if (dataSet)
    {
        dataSet->GetBounds(m_bounds.data());
        m_numberOfPoints = dataSet->GetNumberOfPoints();
        m_numberOfCells = dataSet->GetNumberOfCells();
    }
}

DataObjectPrivate::~DataObjectPrivate()
{
    disconnectAllEvents();
}

vtkAlgorithm * DataObjectPrivate::trivialProducer()
{
    if (!m_trivialProducer)
    {
        auto tp = vtkSmartPointer<vtkTrivialProducer>::New();
        tp->SetOutput(m_dataSet);
        m_trivialProducer = tp;
    }

    return m_trivialProducer;
}

void DataObjectPrivate::addObserver(const QString & eventName, vtkObject & subject, unsigned long tag)
{
    m_namedObserverIds[eventName].insert(&subject, tag);
}

void DataObjectPrivate::disconnectEventGroup(const QString & eventName)
{
    auto && map = m_namedObserverIds[eventName];
    for (auto it = map.begin(); it != map.end(); ++it)
    {
        if (!it.key())    // subject already deleted
            return;

        it.key()->RemoveObserver(it.value());
    }
    m_namedObserverIds.remove(eventName);
}

void DataObjectPrivate::disconnectAllEvents()
{
    for (auto eventName : m_namedObserverIds.keys())
    {
        disconnectEventGroup(eventName);
    }
}

auto DataObjectPrivate::lockEventDeferrals() -> EventDeferralLock
{
    return EventDeferralLock(*this, m_eventDeferralMutex);
}

DataObjectPrivate::EventDeferralLock::EventDeferralLock(DataObjectPrivate & dop, std::recursive_mutex & mutex)
    : m_dop{ dop }
    , m_lock{ std::unique_lock<std::recursive_mutex>(mutex) }
{
}

DataObjectPrivate::EventDeferralLock::EventDeferralLock(EventDeferralLock && other)
    : m_dop{ other.m_dop }
    , m_lock{std::move(other.m_lock)}
{
}

void DataObjectPrivate::EventDeferralLock::addDeferredEvent(const QString & name, EventMemberPointer event)
{
    if (m_dop.m_deferredEvents.contains(name))
    {
        m_dop.m_deferredEvents.insert(name, event);
    }
}

void DataObjectPrivate::EventDeferralLock::deferEvents()
{
    assert(m_dop.m_deferEventsRequests >= 0);
    m_dop.m_deferEventsRequests += 1;
}

bool DataObjectPrivate::EventDeferralLock::isDeferringEvents() const
{
    assert(m_dop.m_deferEventsRequests >= 0);
    return m_dop.m_deferEventsRequests > 0;
}

void DataObjectPrivate::EventDeferralLock::executeDeferredEvents()
{
    assert(m_dop.m_deferEventsRequests > 0);
    m_dop.m_deferEventsRequests -= 1;

    if (isDeferringEvents())
    {
        return;
    }

    // stack is clear, so execute deferred events

    for (auto && eventIt : m_dop.m_deferredEvents)
    {
        eventIt();
    }

    m_dop.m_deferredEvents.clear();
}
