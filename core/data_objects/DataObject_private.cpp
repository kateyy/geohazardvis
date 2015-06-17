#include "DataObject_private.h"

#include <vtkDataSet.h>
#include <vtkInformationIntegerPointerKey.h>
#include <vtkTrivialProducer.h>

#include <core/utility/vtkhelper.h>
#include <core/table_model/QVtkTableModel.h>


vtkInformationKeyMacro(DataObjectPrivate, DataObjectKey, IntegerPointer);


DataObjectPrivate::DataObjectPrivate(DataObject & dataObject, const QString & name, vtkDataSet * dataSet)
    : m_name(name)
    , m_dataSet(dataSet)
    , m_tableModel(nullptr)
    , m_bounds()
    , q_ptr(dataObject)
{
}

DataObjectPrivate::~DataObjectPrivate()
{
    delete m_tableModel;
}

vtkAlgorithm * DataObjectPrivate::trivialProducer()
{
    if (!m_trivialProducer)
    {
        VTK_CREATE(vtkTrivialProducer, tp);
        tp->SetOutput(m_dataSet);
        m_trivialProducer = tp;
    }

    return m_trivialProducer;
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
