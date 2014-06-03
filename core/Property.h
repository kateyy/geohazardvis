#pragma once

#include <memory>


class Input;
class QVtkTableModel;


class Property
{
public:
    Property(std::shared_ptr<Input> input);
    ~Property();

    std::shared_ptr<Input> input();
    std::shared_ptr<const Input> input() const;

    QVtkTableModel * tableModel();

private:
    std::shared_ptr<Input> m_input;
    QVtkTableModel * m_tableModel;
};
