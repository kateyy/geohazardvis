#include "DataObject.h"
#include "DataObject_private.h"

#include <cassert>
#include <type_traits>

#include <vtkAlgorithm.h>
#include <vtkCellData.h>
#include <vtkCharArray.h>
#include <vtkCommand.h>
#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>
#include <vtkInformationIntegerKey.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>

#include <core/context2D_data/Context2DData.h>
#include <core/rendered_data/RenderedData.h>
#include <core/table_model/QVtkTableModel.h>
#include <core/utility/vtkstringhelper.h>


vtkInformationKeyMacro(DataObject, ArrayIsAuxiliaryKey, Integer);


DataObject::DataObject(const QString & name, vtkDataSet * dataSet)
    : d_ptr{ std::make_unique<DataObjectPrivate>(*this, name, dataSet) }
{
    setObjectName(name);

    if (dataSet)
    {
        connectObserver("dataChanged", *dataSet, vtkCommand::ModifiedEvent, *this, &DataObject::_dataChanged);

        connectObserver("attributeArraysChanged", *dataSet->GetPointData(), vtkCommand::ModifiedEvent, *this, &DataObject::_attributeArraysChanged);
        connectObserver("attributeArraysChanged", *dataSet->GetCellData(), vtkCommand::ModifiedEvent, *this, &DataObject::_attributeArraysChanged);
        connectObserver("attributeArraysChanged", *dataSet->GetFieldData(), vtkCommand::ModifiedEvent, *this, &DataObject::_attributeArraysChanged);

        bool resetName = true;
        vtkFieldData * fieldData = dataSet->GetFieldData();
        if (vtkDataArray * nameArray = fieldData->GetArray("Name"))
        {
            QString storedName = vtkArrayToQString(*nameArray);
            resetName = name != storedName;
        }

        if (resetName)
        {
            fieldData->RemoveArray("Name");
            vtkSmartPointer<vtkCharArray> newArray = qstringToVtkArray(name);
            newArray->SetName("Name");
            fieldData->AddArray(newArray);
        }
    }
}

DataObject::~DataObject()
{
    disconnectAllEvents();
}

std::unique_ptr<RenderedData> DataObject::createRendered()
{
    return nullptr;
}

std::unique_ptr<Context2DData> DataObject::createContextData()
{
    return nullptr;
}

const QString & DataObject::name() const
{
    return d_ptr->m_name;
}

vtkDataSet * DataObject::dataSet()
{
    return d_ptr->m_dataSet;
}

const vtkDataSet * DataObject::dataSet() const
{
    return d_ptr->m_dataSet;
}

vtkDataSet * DataObject::processedDataSet()
{
    return d_ptr->m_dataSet;
}

vtkAlgorithmOutput * DataObject::processedOutputPort()
{
    return d_ptr->trivialProducer()->GetOutputPort();
}

const double * DataObject::bounds()
{
    return processedDataSet()->GetBounds();
}

void DataObject::bounds(double b[6])
{
    return processedDataSet()->GetBounds(b);
}

vtkIdType DataObject::numberOfPoints() const
{
    return d_ptr->m_numberOfPoints;
}

vtkIdType DataObject::numberOfCells() const
{
    return d_ptr->m_numberOfCells;
}

QVtkTableModel * DataObject::tableModel()
{
    if (!d_ptr->m_tableModel)
        d_ptr->m_tableModel = createTableModel();

    return d_ptr->m_tableModel.get();
}

void DataObject::addDataArray(vtkDataArray & /*dataArray*/)
{
}

void DataObject::deferEvents()
{
    d_ptr->m_deferEventsRequests += 1;
}

bool DataObject::deferringEvents() const
{
    assert(d_ptr->m_deferEventsRequests >= 0);
    return d_ptr->m_deferEventsRequests > 0;
}

void DataObject::executeDeferredEvents()
{
    d_ptr->m_deferEventsRequests -= 1;

    if (deferringEvents())
        return;

    // stack is clear, so execute deferred events
    d_ptr->executeDeferredEvents();

    // for sub-classes
    process_executeDeferredEvents();
}

DataObject * DataObject::readPointer(vtkInformation & information)
{
    static_assert(sizeof(int*) == sizeof(DataObject*), "");

    if (information.Has(DataObjectPrivate::DataObjectKey()))
    {
        assert(information.Length(DataObjectPrivate::DataObjectKey()) == 1);
        return reinterpret_cast<DataObject *>(information.Get(DataObjectPrivate::DataObjectKey()));
    }

    else return nullptr;
}

void DataObject::storePointer(vtkInformation & information, DataObject * dataObject)
{
    information.Set(DataObjectPrivate::DataObjectKey(), reinterpret_cast<int *>(dataObject), 1);
}

QString DataObject::readName(vtkInformation & information)
{
    if (!information.Has(DataObjectPrivate::NameKey()))
    {
        return{};
    }

    return QString::fromUtf8(information.Get(DataObjectPrivate::NameKey()));
}

void DataObject::storeName(vtkInformation & information, const DataObject & dataObject)
{
    information.Set(DataObjectPrivate::NameKey(), dataObject.name().toUtf8().data());
}

bool DataObject::checkIfBoundsChanged()
{
    if (!dataSet())
        return false;

    decltype(d_ptr->m_bounds) newBounds;
    dataSet()->GetBounds(newBounds.data());

    bool changed = newBounds != d_ptr->m_bounds;

    if (changed)
    {
        d_ptr->m_bounds = newBounds;
    }

    return changed;
}

bool DataObject::checkIfValueRangeChanged()
{
    // this depends on the actual kind of values
    return true;
}

bool DataObject::checkIfStructureChanged()
{
    if (!dataSet())
    {
        return false;
    }

    const vtkIdType newNumPoints = dataSet()->GetNumberOfPoints();
    const vtkIdType newNumCells = dataSet()->GetNumberOfCells();

    bool changed =
        (newNumPoints != d_ptr->m_numberOfPoints)
        || (newNumCells != d_ptr->m_numberOfCells);

    d_ptr->m_numberOfPoints = newNumPoints;
    d_ptr->m_numberOfCells = newNumCells;

    return changed;
}

void DataObject::dataChangedEvent()
{
}

void DataObject::boundsChangedEvent()
{
}

void DataObject::valueRangeChangedEvent()
{
}

void DataObject::structureChangedEvent()
{
}

void DataObject::process_executeDeferredEvents()
{
}

void DataObject::disconnectEventGroup(const QString & eventName)
{
    d_ptr->disconnectEventGroup(eventName);
}

void DataObject::disconnectAllEvents()
{
}

void DataObject::_dataChanged()
{
    if (deferringEvents())
    {
        d_ptr->addDeferredEvent("dataChanged", std::bind(&DataObject::_dataChanged, this));
        return;
    }

    dataChangedEvent();

    emit dataChanged();

    if (checkIfBoundsChanged())
    {
        boundsChangedEvent();

        emit boundsChanged();
    }

    if (checkIfValueRangeChanged())
    {
        valueRangeChangedEvent();

        emit valueRangeChanged();
    }

    if (checkIfStructureChanged())
    {
        structureChangedEvent();

        emit structureChanged();
    }
}

void DataObject::_attributeArraysChanged()
{
    if (deferringEvents())
    {
        d_ptr->addDeferredEvent("_attributeArraysChanged", std::bind(&DataObject::_attributeArraysChanged, this));
        return;
    }

    emit attributeArraysChanged();
}

void DataObject::addObserver(const QString & eventName, vtkObject & subject, unsigned long tag)
{
    d_ptr->m_namedObserverIds[eventName].insert(&subject, tag);
}
