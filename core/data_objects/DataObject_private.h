#pragma once

#include <QMap>
#include <QString>

#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>


class vtkAlgorithm;
class vtkDataSet;
class vtkInformationIntegerPointerKey;
class vtkObject;

class DataObject;
class QVtkTableModel;


class DataObjectPrivate
{
public:
    DataObjectPrivate(DataObject & dataObject, const QString & name, vtkDataSet * dataSet);

    virtual ~DataObjectPrivate();

    static vtkInformationIntegerPointerKey * DataObjectKey();

    vtkAlgorithm * trivialProducer();

    void disconnectEventGroup(const QString & eventName);
    void disconnectAllEvents();

public:
    QString m_name;

    vtkSmartPointer<vtkDataSet> m_dataSet;
    QVtkTableModel * m_tableModel;

    double m_bounds[6];

    QMap<QString, QMap<vtkWeakPointer<vtkObject>, unsigned long>> m_namedObserverIds;

protected:
    DataObject & q_ptr;

private:
    vtkSmartPointer<vtkAlgorithm> m_trivialProducer;
};
