#include "DataObject_private.h"

#include <vtkDataSet.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkInformationIntegerPointerKey.h>
#include <vtkTrivialProducer.h>

#include <core/vtkhelper.h>
#include <core/table_model/QVtkTableModel.h>


vtkInformationKeyMacro(DataObjectPrivate, DataObjectKey, IntegerPointer);


DataObjectPrivate::DataObjectPrivate(DataObject & dataObject, QString name, vtkDataSet * dataSet)
    : q_ptr(dataObject)
    , m_name(name)
    , m_dataSet(dataSet)
    , m_tableModel(nullptr)
    , m_bounds()
    , m_vtkQtConnect(vtkSmartPointer<vtkEventQtSlotConnect>::New())
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
