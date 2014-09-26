#pragma once

#include <QString>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkInformationStringKey;
class vtkInformationIntegerKey;
class vtkDataSet;
class QVtkTableModel;
class RenderedData;


/** Base class representing loaded data. */
class CORE_API DataObject
{
public:
    DataObject(QString name, vtkDataSet * dataSet);
    virtual ~DataObject() = 0;

    /** @return true if this is a 3D geometry (and false if it's image/2D data) */
    virtual bool is3D() const = 0;

    /** create a rendered instance */
    virtual RenderedData * createRendered() = 0;

    QString name() const;
    virtual QString dataTypeName() const = 0;

    vtkDataSet * dataSet();
    const vtkDataSet * dataSet() const;

    const double * bounds();

    QVtkTableModel * tableModel();

    static vtkInformationStringKey * NameKey();
    static vtkInformationIntegerKey * ArrayIsAuxiliaryKey();

protected:
    virtual QVtkTableModel * createTableModel() = 0;

private:
    QString m_name;

    vtkSmartPointer<vtkDataSet> m_dataSet;
    QVtkTableModel * m_tableModel;
};
