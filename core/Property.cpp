#include "Property.h"

#include <vtkDataSet.h>

#include "Input.h"

#include "gui/QVtkTableModel.h"


Property::Property(std::shared_ptr<Input> input)
: m_input(input)
, m_tableModel(new QVtkTableModel())
{
    m_tableModel->showData(m_input->data());
}

Property::~Property()
{
    delete m_tableModel;
}

std::shared_ptr<Input> Property::input()
{
    return m_input;
}

std::shared_ptr<const Input> Property::input() const
{
    return m_input;
}

QVtkTableModel * Property::tableModel()
{
    return m_tableModel;
}