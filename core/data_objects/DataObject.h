#pragma once

#include <memory>

#include <QString>

#include <core/core_api.h>


class Input;
class QVtkTableModel;


/** Base class representing loaded data. */
class CORE_API DataObject
{
public:
    DataObject(std::shared_ptr<Input> input);
    virtual ~DataObject() = 0;

    virtual QString dataTypeName() const = 0;

    std::shared_ptr<Input> input();
    std::shared_ptr<const Input> input() const;

    QVtkTableModel * tableModel();

private:
    std::shared_ptr<Input> m_input;
    QVtkTableModel * m_tableModel;
};
