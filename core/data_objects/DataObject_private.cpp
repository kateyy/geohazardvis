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

#include "DataObject_private.h"

#include <cassert>

#include <vtkAlgorithmOutput.h>
#include <vtkDataSet.h>
#include <vtkInformationIntegerPointerKey.h>
#include <vtkInformationStringKey.h>
#include <vtkPassThrough.h>
#include <vtkTrivialProducer.h>



vtkInformationKeyMacro(DataObjectPrivate, DATA_OBJECT, IntegerPointer);
vtkInformationKeyMacro(DataObjectPrivate, DATA_OBJECT_NAME, String);


DataObjectPrivate::DataObjectPrivate(DataObject & dataObject, const QString & name, vtkDataSet * dataSet)
    : m_name{ name }
    , m_dataSet{ dataSet }
    , m_tableModel{}
    , m_bounds{}
    , m_numberOfPoints{ 0 }
    , m_numberOfCells{ 0 }
    , m_inCopyStructure{ false }
    , q_ptr{ dataObject }
    , m_nextProcessingStepId{ 0 }
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
    assert(m_deferEventsRequests == 0);

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

vtkAlgorithm * DataObjectPrivate::pipelineEndPoint()
{
    if (!m_processedPassThrough)
    {
        // Initial hard-coded pipeline. When extending it using injectPostProcessingStep,
        // updatePipeline() will set a different input here.
        m_processedPassThrough = vtkSmartPointer<vtkPassThrough>::New();
        m_processedPassThrough->SetInputConnection(q_ptr.processedOutputPortInternal());
    }

    return m_processedPassThrough;
}

std::pair<bool, unsigned int> DataObjectPrivate::injectPostProcessingStep(const PostProcessingStep & postProcessingStep)
{
    const auto newId = getNextProcessingStepId();
    m_postProcessingSteps.emplace_back(newId, postProcessingStep);
    updatePipeline();

    return std::make_pair(true, newId);
}

bool DataObjectPrivate::erasePostProcessingStep(const unsigned int id)
{
    const auto it = std::find_if(m_postProcessingSteps.begin(), m_postProcessingSteps.end(),
        [id] (const std::pair<unsigned int, PostProcessingStep> & step)
    {
        return step.first == id;
    });

    if (it == m_postProcessingSteps.end())
    {
        return false;
    }

    m_postProcessingSteps.erase(it);
    updatePipeline();
    releaseProcessingStepId(id);

    return true;
}

void DataObjectPrivate::updatePipeline()
{
    auto upstream = q_ptr.processedOutputPortInternal();
    assert(upstream);

    vtkSmartPointer<vtkAlgorithmOutput> currentUpstream = upstream;

    for (const auto & step : m_postProcessingSteps)
    {
        step.second.pipelineHead->SetInputConnection(currentUpstream);
        currentUpstream = step.second.pipelineTail->GetOutputPort();
    }

    pipelineEndPoint()->SetInputConnection(currentUpstream);
}

void DataObjectPrivate::addObserver(const QString & eventName, vtkObject & subject, unsigned long tag)
{
    m_namedObserverIds[eventName].emplace(&subject, tag);
}

void DataObjectPrivate::disconnectEventGroup(const QString & eventName)
{
    auto observersForEventIt = m_namedObserverIds.find(eventName);
    if (observersForEventIt == m_namedObserverIds.end())
    {
        assert(false);
        return;
    }

    disconnectEventGroup_internal(observersForEventIt->second);
    m_namedObserverIds.erase(observersForEventIt);
}

void DataObjectPrivate::disconnectEventGroup_internal(
    const std::map<vtkWeakPointer<vtkObject>, unsigned long> & observersToDisconnect) const
{
    for (auto && observerAndId : observersToDisconnect)
    {
        if (!observerAndId.first)    // subject already deleted
        {
            continue;
        }

        observerAndId.first->RemoveObserver(observerAndId.second);
    }
}

unsigned int DataObjectPrivate::getNextProcessingStepId()
{
    if (!m_freedProcessingStepIds.empty())
    {
        const auto id = m_freedProcessingStepIds.back();
        m_freedProcessingStepIds.pop_back();
        return id;
    }

    return m_nextProcessingStepId++;
}

void DataObjectPrivate::releaseProcessingStepId(const unsigned int id)
{
    m_freedProcessingStepIds.push_back(id);
}

void DataObjectPrivate::disconnectAllEvents()
{
    for (auto && observersForEvent : m_namedObserverIds)
    {
        disconnectEventGroup_internal(observersForEvent.second);
    }
    m_namedObserverIds.clear();
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
    , m_lock{ std::move(other.m_lock) }
{
}

void DataObjectPrivate::EventDeferralLock::addDeferredEvent(const std::string & name, EventMemberPointer && event)
{
    m_dop.m_deferredEvents.emplace(name, std::forward<EventMemberPointer>(event));
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
        eventIt.second();
    }

    m_dop.m_deferredEvents.clear();
}
