#include "DataObject.h"
#include "DataObject_private.h"

#include <cassert>
#include <type_traits>

#include <vtkAlgorithm.h>
#include <vtkCellData.h>
#include <vtkCharArray.h>
#include <vtkDataSet.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkInformationIntegerKey.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>

#include <core/utility/vtkhelper.h>
#include <core/utility/vtkstringhelper.h>


vtkInformationKeyMacro(DataObject, NameKey, String);
vtkInformationKeyMacro(DataObject, ArrayIsAuxiliaryKey, Integer);


DataObject::DataObject(const QString & name, vtkDataSet * dataSet)
    : d_ptr(new DataObjectPrivate(*this, name, dataSet))
{
    if (dataSet)
    {
        dataSet->GetBounds(d_ptr->m_bounds);

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
    delete d_ptr;
}

RenderedData * DataObject::createRendered()
{
    return nullptr;
}

Context2DData * DataObject::createContextData()
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

QVtkTableModel * DataObject::tableModel()
{
    if (!d_ptr->m_tableModel)
        d_ptr->m_tableModel = createTableModel();

    return d_ptr->m_tableModel;
}

void DataObject::addDataArray(vtkDataArray * /*dataArray*/)
{
}

DataObject * DataObject::getDataObject(vtkInformation * information)
{
    static_assert(sizeof(int*) == sizeof(DataObject*), "");

    if (information->Has(DataObjectPrivate::DataObjectKey()))
    {
        assert(information->Length(DataObjectPrivate::DataObjectKey()) == 1);
        return reinterpret_cast<DataObject *>(information->Get(DataObjectPrivate::DataObjectKey()));
    }

    else return nullptr;
}

void DataObject::setDataObject(vtkInformation * information, DataObject * dataObject)
{
    information->Set(DataObjectPrivate::DataObjectKey(), reinterpret_cast<int *>(dataObject), 1);
}

bool DataObject::checkIfBoundsChanged()
{
    double newBounds[6];

    if (!dataSet())
        return true;

    dataSet()->GetBounds(newBounds);

    bool changed = false;
    for (int i = 0; i < 6; ++i)
        changed = changed || (d_ptr->m_bounds[i] != newBounds[i]);

    if (changed)
        for (int i = 0; i < 6; ++i)
            d_ptr->m_bounds[i] = newBounds[i];

    return changed;
}

bool DataObject::checkIfValueRangeChanged()
{
    // this depends on the actual kind of values
    return true;
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

void DataObject::disconnectEventGroup(const QString & eventName)
{
    d_ptr->disconnectEventGroup(eventName);
}

void DataObject::disconnectAllEvents()
{
    d_ptr->disconnectAllEvents();
}

void DataObject::_dataChanged()
{
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
}

void DataObject::_attributeArraysChanged(vtkObject * /*caller*/, unsigned long /*event*/, void * /*callData*/)
{
    emit attributeArraysChanged();
}

void DataObject::addObserver(const QString & eventName, vtkObject & subject, unsigned long tag)
{
    d_ptr->m_namedObserverIds[eventName].insert(&subject, tag);
}
