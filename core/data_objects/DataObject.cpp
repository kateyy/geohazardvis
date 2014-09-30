#include "DataObject.h"

#include <vtkInformationStringKey.h>
#include <vtkInformationIntegerKey.h>

#include <vtkDataSet.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkTrivialProducer.h>

#include <core/table_model/QVtkTableModel.h>


vtkInformationKeyMacro(DataObject, NameKey, String);
vtkInformationKeyMacro(DataObject, ArrayIsAuxiliaryKey, Integer);

class DataObjectPrivate
{
public:
    DataObjectPrivate(DataObject & dataObject, QString name, vtkDataSet * dataSet)
        : q_ptr(dataObject)
        , m_name(name)
        , m_dataSet(dataSet)
        , m_tableModel(nullptr)
        , m_bounds()
        , m_vtkQtConnect(vtkSmartPointer<vtkEventQtSlotConnect>::New())
    {
    }
    
    ~DataObjectPrivate()
    {
        q_ptr.blockSignals(false);
    }

    static vtkInformationIntegerPointerKey * DataObjectKey();

    DataObject & q_ptr;

    QString m_name;

    vtkSmartPointer<vtkDataSet> m_dataSet;
    QVtkTableModel * m_tableModel;

    vtkSmartPointer<vtkTrivialProducer> m_trivialProducer;

    double m_bounds[6];

    vtkSmartPointer<vtkEventQtSlotConnect> m_vtkQtConnect;
};

vtkInformationKeyMacro(DataObjectPrivate, DataObjectKey, IntegerPointer);


DataObject::DataObject(QString name, vtkDataSet * dataSet)
    : d_ptr(new DataObjectPrivate(*this, name, dataSet))
{
    if (dataSet)
    {
        dataSet->GetBounds(d_ptr->m_bounds);

        d_ptr->m_vtkQtConnect->Connect(dataSet, vtkCommand::ModifiedEvent, this, SLOT(_dataChanged()));
    }
}

DataObject::~DataObject()
{
    delete d_ptr->m_tableModel;
}

QString DataObject::name() const
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
    if (!d_ptr->m_trivialProducer)
    {
        d_ptr->m_trivialProducer = vtkSmartPointer<vtkTrivialProducer>::New();
        d_ptr->m_trivialProducer->SetOutput(d_ptr->m_dataSet);
    }

    return d_ptr->m_trivialProducer->GetOutputPort();
}

const double * DataObject::bounds()
{
    return dataSet()->GetBounds();
}

QVtkTableModel * DataObject::tableModel()
{
    if (!d_ptr->m_tableModel)
        d_ptr->m_tableModel = createTableModel();

    return d_ptr->m_tableModel;
}

vtkEventQtSlotConnect * DataObject::vtkQtConnect()
{
    return d_ptr->m_vtkQtConnect;
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
