#pragma once

#include <array>
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
class vtkInformationStringKey;
class vtkObject;

class DataObject;


class DataObjectPrivate
{
public:
    DataObjectPrivate(DataObject & dataObject, const QString & name, vtkDataSet * dataSet);

    virtual ~DataObjectPrivate();

    static vtkInformationIntegerPointerKey * DataObjectKey();
    static vtkInformationStringKey * NameKey();

    vtkAlgorithm * trivialProducer();

    void disconnectEventGroup(const QString & eventName);
    void disconnectAllEvents();

public:
    QString m_name;

    vtkSmartPointer<vtkDataSet> m_dataSet;
    std::unique_ptr<QVtkTableModel> m_tableModel;

    std::array<double, 6> m_bounds;
    vtkIdType m_numberOfPoints;
    vtkIdType m_numberOfCells;

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
