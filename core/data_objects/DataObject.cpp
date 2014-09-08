#include "DataObject.h"

#include <vtkInformationStringKey.h>

#include <vtkDataSet.h>

#include <core/QVtkTableModel.h>


vtkInformationKeyMacro(DataObject, NameKey, String);

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

QVtkTableModel * DataObject::createTableModel()
{
    QVtkTableModel * model = new QVtkTableModel;

    model->showData(dataSet());

    return model;
}
