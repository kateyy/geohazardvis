#include "DataObject.h"

#include <vtkInformationStringKey.h>

#include <vtkDataSet.h>

#include <core/Input.h>

#include <core/QVtkTableModel.h>


vtkInformationKeyMacro(DataObject, NameKey, String);

DataObject::DataObject(std::shared_ptr<Input> input)
    : m_dataSet(input->data())
    , m_input(input)
    , m_tableModel(new QVtkTableModel())
{
    m_tableModel->showData(m_input->data());
}

DataObject::~DataObject()
{
    delete m_tableModel;
}

QString DataObject::name() const
{
    if (!m_input)
        return QString();

    return QString::fromStdString(m_input->name);
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
