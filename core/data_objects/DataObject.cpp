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


DataObject::DataObject(QString name, vtkDataSet * dataSet)
    : m_name(name)
    , m_dataSet(dataSet)
    , m_tableModel(nullptr)
    , m_bounds()
    , m_vtkQtConnect(vtkSmartPointer<vtkEventQtSlotConnect>::New())
{
    if (dataSet)
    {
        dataSet->GetBounds(m_bounds);

        m_vtkQtConnect->Connect(dataSet, vtkCommand::ModifiedEvent, this, SLOT(_dataChanged()));
    }
}

DataObject::~DataObject()
{
    delete m_tableModel;
}

QString DataObject::name() const
{
    return m_name;
}

vtkDataSet * DataObject::dataSet()
{
    return m_dataSet;
}

const vtkDataSet * DataObject::dataSet() const
{
    return m_dataSet;
}

vtkDataSet * DataObject::processedDataSet()
{
    return m_dataSet;
}

vtkAlgorithmOutput * DataObject::processedOutputPort()
{
    if (!m_trivialProducer)
    {
        m_trivialProducer = vtkSmartPointer<vtkTrivialProducer>::New();
        m_trivialProducer->SetOutput(m_dataSet);
    }

    return m_trivialProducer->GetOutputPort();
}

const double * DataObject::bounds()
{
    return dataSet()->GetBounds();
}

QVtkTableModel * DataObject::tableModel()
{
    if (!m_tableModel)
        m_tableModel = createTableModel();

    return m_tableModel;
}

vtkEventQtSlotConnect * DataObject::vtkQtConnect()
{
    return m_vtkQtConnect;
}

bool DataObject::checkIfBoundsChanged()
{
    double newBounds[6];

    if (!dataSet())
        return true;

    dataSet()->GetBounds(newBounds);

    bool changed = false;
    for (int i = 0; i < 6; ++i)
        changed = changed || (m_bounds[i] != newBounds[i]);

    if (changed)
        for (int i = 0; i < 6; ++i)
            m_bounds[i] = newBounds[i];

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
