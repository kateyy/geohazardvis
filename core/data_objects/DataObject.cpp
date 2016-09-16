#include "DataObject.h"
#include "DataObject_private.h"

#include <array>
#include <cassert>
#include <type_traits>

#include <QStringList>

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


namespace
{
    const char * nameAttributeName()
    {
        static const char * const _name = "Name";
        return _name;
    }
}


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
        if (vtkDataArray * nameArray = fieldData->GetArray(nameAttributeName()))
        {
            const QString storedName = vtkArrayToQString(*nameArray);
            resetName = name != storedName;
        }

        if (resetName)
        {
            fieldData->RemoveArray(nameAttributeName());
            vtkSmartPointer<vtkCharArray> newArray = qstringToVtkArray(name);
            newArray->SetName(nameAttributeName());
            fieldData->AddArray(newArray);
        }
    }
}

DataObject::~DataObject() = default;

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

vtkAlgorithmOutput * DataObject::processedOutputPort()
{
    return d_ptr->trivialProducer()->GetOutputPort();
}

vtkDataSet * DataObject::processedDataSet()
{
    auto passThrough = d_ptr->processedPassThrough();
    passThrough->SetInputConnection(processedOutputPort());
    passThrough->Update();
    return vtkDataSet::SafeDownCast(passThrough->GetOutputDataObject(0));
}

const DataBounds & DataObject::bounds() const
{
    return d_ptr->m_bounds;
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
    {
        d_ptr->m_tableModel = createTableModel();
    }

    return d_ptr->m_tableModel.get();
}

void DataObject::addDataArray(vtkDataArray & /*dataArray*/)
{
}

void DataObject::clearAttributes()
{
    const std::array<QStringList, 3> intrinsicAttributes = [this] ()
    {
        std::array<QStringList, 3> attrNames;   // field, point, cell
        addIntrinsicAttributes(attrNames[0], attrNames[1], attrNames[2]);

        attrNames[0].prepend(nameAttributeName());

        return attrNames;
    }();

    if (!dataSet())
    {
        return;
    }

    auto & ds = *dataSet();

    const auto cleanupAttributes = [] (vtkFieldData & fd, const QStringList & intrinsicAttributes)
    {
        int i = 0;
        while (i < fd.GetNumberOfArrays())
        {
            auto array = fd.GetAbstractArray(i);
            if (intrinsicAttributes.contains(QString::fromUtf8(array->GetName())))
            {
                ++i;
                continue;
            }
            fd.RemoveArray(i);
        }
    };

    cleanupAttributes(*ds.GetFieldData(), intrinsicAttributes[0]);
    cleanupAttributes(*ds.GetPointData(), intrinsicAttributes[1]);
    cleanupAttributes(*ds.GetCellData(), intrinsicAttributes[2]);

}

void DataObject::deferEvents()
{
    d_ptr->lockEventDeferrals().deferEvents();
}

void DataObject::executeDeferredEvents()
{
    d_ptr->lockEventDeferrals().executeDeferredEvents();
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

DataObjectPrivate & DataObject::dPtr()
{
    return *d_ptr;
}

void DataObject::addIntrinsicAttributes(
    QStringList & /*fieldAttributes*/,
    QStringList & /*pointAttributes*/,
    QStringList & /*cellAttributes*/)
{
}

bool DataObject::checkIfBoundsChanged()
{
    if (!dataSet())
    {
        return false;
    }

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
    auto lock = d_ptr->lockEventDeferrals();

    if (lock.isDeferringEvents())
    {
        lock.addDeferredEvent("dataChanged", std::bind(&DataObject::_dataChanged, this));
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
    auto lock = d_ptr->lockEventDeferrals();

    if (lock.isDeferringEvents())
    {
        lock.addDeferredEvent("_attributeArraysChanged", std::bind(&DataObject::_attributeArraysChanged, this));
        return;
    }

    emit attributeArraysChanged();
}

void DataObject::addObserver(const QString & eventName, vtkObject & subject, unsigned long tag)
{
    d_ptr->addObserver(eventName, subject, tag);
}
