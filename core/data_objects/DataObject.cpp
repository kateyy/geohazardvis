#include "DataObject.h"

#include <vtkInformationStringKey.h>
#include <vtkInformationIntegerKey.h>

#include <vtkDataSet.h>

#include <core/table_model/QVtkTableModel.h>


vtkInformationKeyMacro(DataObject, NameKey, String);
vtkInformationKeyMacro(DataObject, ArrayIsAuxiliaryKey, Integer);


DataObject::DataObject(QString name, vtkDataSet * dataSet)
    : m_name(name)
    , m_dataSet(dataSet)
    , m_tableModel(nullptr)
{
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
