#pragma once

#include <memory>


class Input;
class QVtkTableModel;


/** Base class representing loaded data. */
class DataObject
{
public:
    DataObject(std::shared_ptr<Input> input);
    virtual ~DataObject() = 0;

    std::shared_ptr<Input> input();
    std::shared_ptr<const Input> input() const;

    QVtkTableModel * tableModel();

private:
    std::shared_ptr<Input> m_input;
    QVtkTableModel * m_tableModel;
};
