#include "DataObject.h"

#include <vtkInformationStringKey.h>

#include <vtkDataSet.h>

#include <core/QVtkTableModel.h>


vtkInformationKeyMacro(DataObject, NameKey, String);

DataObject::DataObject(QString name, vtkDataSet * dataSet)
    : m_name(name)
    , m_dataSet(dataSet)
    , m_tableModel(new QVtkTableModel())
{
    m_tableModel->showData(dataSet);
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
    return m_tableModel;
}
