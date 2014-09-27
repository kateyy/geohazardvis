#pragma once

#include <QString>
#include <QObject>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkInformationStringKey;
class vtkInformationIntegerKey;
class vtkDataSet;
class vtkEventQtSlotConnect;
class QVtkTableModel;
class RenderedData;


/** Base class representing loaded data. */
class CORE_API DataObject : public QObject
{
    Q_OBJECT

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

signals:
    void dataChanged();
    void boundsChanged();
    void valueRangeChanged();

protected:
    virtual QVtkTableModel * createTableModel() = 0;

    vtkEventQtSlotConnect * vtkQtConnect();
protected slots:
    void _dataChanged();

protected:
    /** when data set values changed, check whether this also affects the bounds*/
    virtual bool checkIfBoundsChanged();
    virtual bool checkIfValueRangeChanged();

    virtual void dataChangedEvent();
    virtual void boundsChangedEvent();
    virtual void valueRangeChangedEvent();

private:
    QString m_name;

    vtkSmartPointer<vtkDataSet> m_dataSet;
    QVtkTableModel * m_tableModel;

    double m_bounds[6];

    vtkSmartPointer<vtkEventQtSlotConnect> m_vtkQtConnect;
};
