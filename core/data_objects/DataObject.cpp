#include "DataObject.h"

#include <vtkDataSet.h>

#include <core/Input.h>

#include <core/QVtkTableModel.h>


DataObject::DataObject(std::shared_ptr<Input> input)
: m_input(input)
, m_tableModel(new QVtkTableModel())
{
    m_tableModel->showData(m_input->data());
}

DataObject::~DataObject()
{
    delete m_tableModel;
}

std::shared_ptr<Input> DataObject::input()
{
    return m_input;
}

std::shared_ptr<const Input> DataObject::input() const
{
    return m_input;
}

QVtkTableModel * DataObject::tableModel()
{
    return m_tableModel;
}
