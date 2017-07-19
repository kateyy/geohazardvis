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
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <string>
#include <vector>

#include <QString>

#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

#include <core/data_objects/DataObject.h>
#include <core/table_model/QVtkTableModel.h>
#include <core/utility/DataExtent.h>


class vtkAlgorithm;
class vtkDataSet;
class vtkInformationIntegerPointerKey;
class vtkInformationStringKey;
class vtkObject;

class DataObject;


class CORE_API DataObjectPrivate
{
public:
    DataObjectPrivate(DataObject & dataObject, const QString & name, vtkDataSet * dataSet);

    virtual ~DataObjectPrivate();

    static vtkInformationIntegerPointerKey * DATA_OBJECT();
    static vtkInformationStringKey * DATA_OBJECT_NAME();

    vtkAlgorithm * trivialProducer();
    /** Persistent processing pipeline end point. This is a pass-through in the simplest case
     * and can be extended by overriding DataObject::processedOutputPortInternal or using
     * injectPostProcessingStep. */
    vtkAlgorithm * pipelineEndPoint();
    using PostProcessingStep = DataObject::PostProcessingStep;
    std::pair<bool, unsigned int> injectPostProcessingStep(const PostProcessingStep & postProcessingStep);
    bool erasePostProcessingStep(unsigned int id);

    void addObserver(const QString & eventName, vtkObject & subject, unsigned long tag);
    void disconnectEventGroup(const QString & eventName);
    void disconnectAllEvents();

private:
    void disconnectEventGroup_internal(
        const std::map<vtkWeakPointer<vtkObject>, unsigned long> & observersToDisconnect) const;

public:
    QString m_name;

    vtkSmartPointer<vtkDataSet> m_dataSet;

    std::unique_ptr<QVtkTableModel> m_tableModel;

    DataBounds m_bounds;
    vtkIdType m_numberOfPoints;
    vtkIdType m_numberOfCells;

    /** Workaround flag for vtkPolyData::CopyStructure triggering ModifiedEvent after copying
    points, but not after copying cell arrays */
    bool m_inCopyStructure;

    using EventMemberPointer = std::function<void()>;

    class CORE_API EventDeferralLock
    {
    public:
        explicit EventDeferralLock(DataObjectPrivate & dop, std::recursive_mutex & mutex);
        EventDeferralLock(EventDeferralLock && other);

        void addDeferredEvent(const std::string & name, EventMemberPointer && event);
        void deferEvents();
        bool isDeferringEvents() const;
        void executeDeferredEvents();

        EventDeferralLock(const EventDeferralLock &) = delete;
        EventDeferralLock & operator=(const EventDeferralLock &) = delete;
    private:
        DataObjectPrivate & m_dop;
        std::unique_lock<std::recursive_mutex> m_lock;
    };

    EventDeferralLock lockEventDeferrals();

protected:
    DataObject & q_ptr;

private:
    void updatePipeline();
    unsigned int getNextProcessingStepId();
    void releaseProcessingStepId(unsigned int id);

private:
    /** Producer that is used as pipeline source. */
    vtkSmartPointer<vtkAlgorithm> m_trivialProducer;
    /** List of post processing steps were injected between m_trivialProducer and 
     * m_processedPassThrough. */
    std::vector<std::pair<unsigned int, DataObject::PostProcessingStep>> m_postProcessingSteps;
    /** Endpoint of processing pipeline that are possibly extended within subclasses or by injected
     * processing steps. */
    vtkSmartPointer<vtkAlgorithm> m_processedPassThrough;

    unsigned int m_nextProcessingStepId;
    std::vector<unsigned int> m_freedProcessingStepIds;

    std::map<QString, std::map<vtkWeakPointer<vtkObject>, unsigned long>> m_namedObserverIds;

    friend class EventDeferralLock;
    std::recursive_mutex m_eventDeferralMutex;
    int m_deferEventsRequests;
    std::map<std::string, EventMemberPointer> m_deferredEvents;
};
