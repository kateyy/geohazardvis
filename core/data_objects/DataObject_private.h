#pragma once

#include <QString>

#include <vtkSmartPointer.h>


class vtkDataSet;
class vtkEventQtSlotConnect;
class vtkInformationIntegerPointerKey;
class vtkAlgorithm;

class DataObject;
class QVtkTableModel;


class DataObjectPrivate
{
public:
    DataObjectPrivate(DataObject & dataObject, const QString & name, vtkDataSet * dataSet);

    virtual ~DataObjectPrivate();

    static vtkInformationIntegerPointerKey * DataObjectKey();

    vtkAlgorithm * trivialProducer();

public:
    QString m_name;

    vtkSmartPointer<vtkDataSet> m_dataSet;
    QVtkTableModel * m_tableModel;

    double m_bounds[6];

    vtkSmartPointer<vtkEventQtSlotConnect> m_vtkQtConnect;

protected:
    DataObject & q_ptr;

private:
    vtkSmartPointer<vtkAlgorithm> m_trivialProducer;
};
