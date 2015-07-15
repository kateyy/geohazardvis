#pragma once

#include <functional>
#include <memory>

#include <QMap>
#include <QString>

#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

#include <core/table_model/QVtkTableModel.h>


class vtkAlgorithm;
class vtkDataSet;
class vtkInformationIntegerPointerKey;
class vtkObject;

class DataObject;


class DataObjectPrivate
{
public:
    DataObjectPrivate(DataObject & dataObject, const QString & name, vtkDataSet * dataSet);

    virtual ~DataObjectPrivate();

    static vtkInformationIntegerPointerKey * DataObjectKey();

    vtkAlgorithm * trivialProducer();

    void disconnectEventGroup(const QString & eventName);
    void disconnectAllEvents();

public:
    QString m_name;

    vtkSmartPointer<vtkDataSet> m_dataSet;
    std::unique_ptr<QVtkTableModel> m_tableModel;

    double m_bounds[6];

    QMap<QString, QMap<vtkWeakPointer<vtkObject>, unsigned long>> m_namedObserverIds;

    int m_deferEventsRequests;
    using EventMemberPointer = std::function<void()>;
    void addDeferredEvent(const QString & name, const EventMemberPointer & event);
    void executeDeferredEvents();

protected:
    DataObject & q_ptr;

private:
    vtkSmartPointer<vtkAlgorithm> m_trivialProducer;
    QMap<QString, EventMemberPointer> m_deferredEvents;
};
