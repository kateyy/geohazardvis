#include "InputRepresentation.h"

#include <vtkDataSet.h>

#include "core/Input.h"

#include "QVtkTableModel.h"


InputRepresentation::InputRepresentation(std::shared_ptr<Input> input)
: m_input(input)
, m_tableModel(new QVtkTableModel())
{
    m_tableModel->showData(m_input->data());
}

InputRepresentation::~InputRepresentation()
{
    delete m_tableModel;
}

std::shared_ptr<const Input> InputRepresentation::input() const
{
    return m_input;
}

QVtkTableModel * InputRepresentation::tableModel()
{
    return m_tableModel;
}