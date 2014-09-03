#pragma once

#include <memory>

#include <QString>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkInformationStringKey;
class Input;
class vtkDataSet;
class QVtkTableModel;
class RenderedData;


/** Base class representing loaded data. */
class CORE_API DataObject
{
public:
    DataObject(std::shared_ptr<Input> input);
    virtual ~DataObject() = 0;

    /** create a rendered instance */
    virtual RenderedData * createRendered() = 0;

    QString name() const;
    virtual QString dataTypeName() const = 0;

    vtkDataSet * dataSet();
    const vtkDataSet * dataSet() const;

    const double * bounds();

    QVtkTableModel * tableModel();

    static vtkInformationStringKey * NameKey();

private:
    vtkSmartPointer<vtkDataSet> m_dataSet;
    std::shared_ptr<Input> m_input;
    QVtkTableModel * m_tableModel;
};
