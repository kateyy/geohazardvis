#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>

#include <QString>

#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

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

    static vtkInformationIntegerPointerKey * DataObjectKey();
    static vtkInformationStringKey * NameKey();

    vtkAlgorithm * trivialProducer();
    /** Filter that shallow copies and passes through the input.
    This allows to provide a persistent data sets for varying upstreams. */
    vtkAlgorithm * processedPassThrough();

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

        void addDeferredEvent(const QString & name, EventMemberPointer event);
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
    vtkSmartPointer<vtkAlgorithm> m_trivialProducer;
    vtkSmartPointer<vtkAlgorithm> m_processedPassThrough;
    std::map<QString, std::map<vtkWeakPointer<vtkObject>, unsigned long>> m_namedObserverIds;

    friend class EventDeferralLock;
    std::recursive_mutex m_eventDeferralMutex;
    int m_deferEventsRequests;
    std::map<QString, EventMemberPointer> m_deferredEvents;
};
