#pragma once

#include <QObject>

#include <core/core_api.h>


class vtkInformation;
class vtkInformationStringKey;
class vtkInformationIntegerKey;
class vtkDataArray;
class vtkDataSet;
class vtkAlgorithmOutput;
class vtkObject;
class QVtkTableModel;
class RenderedData;
class Context2DData;

class DataObjectPrivate;


/** Base class representing loaded data. */
class CORE_API DataObject : public QObject
{
    Q_OBJECT

public:
    DataObject(const QString & name, vtkDataSet * dataSet);
    virtual ~DataObject();

    /** @return true if this is a 3D geometry (and false if it's image/2D data) */
    virtual bool is3D() const = 0;

    /** create a visualization instance; Subclasses may implement one or both
        -> createRendered: flexible/generic rendering 
        -> createContextData: for specialized views (e.g., plots)
    */
    virtual RenderedData * createRendered();
    virtual Context2DData * createContextData();

    const QString & name() const;
    virtual const QString & dataTypeName() const = 0;

    vtkDataSet * dataSet();
    const vtkDataSet * dataSet() const;

    virtual vtkDataSet * processedDataSet();
    virtual vtkAlgorithmOutput * processedOutputPort();

    const double * bounds();
    void bounds(double b[6]);

    QVtkTableModel * tableModel();

    /** assign some kind of data array to my indexes.
        Let subclasses decide how to proceed with it; ignores this call by default */
    virtual void addDataArray(vtkDataArray * dataArray);

    static vtkInformationStringKey * NameKey();
    static vtkInformationIntegerKey * ArrayIsAuxiliaryKey();

    static DataObject * getDataObject(vtkInformation * information);
    static void setDataObject(vtkInformation * information, DataObject * dataObject);

signals:
    void dataChanged();
    void boundsChanged();
    void valueRangeChanged();
    void attributeArraysChanged();

protected:
    virtual QVtkTableModel * createTableModel() = 0;

    template<typename U, typename T>
    void connectObserver(const QString & eventName, vtkObject & subject,
        unsigned long event, U & observer, void(T::* slot)(void));
    template<typename U, typename T>
    void connectObserver(const QString & eventName, vtkObject & subject,
        unsigned long event, U & observer, void(T::* slot)(vtkObject*, unsigned long, void*));
    void disconnectEventGroup(const QString & eventName);
    void disconnectAllEvents();

    void _dataChanged();
    void _attributeArraysChanged(vtkObject * caller, unsigned long event, void * callData);

protected:
    /** when data set values changed, check whether this also affects the bounds*/
    virtual bool checkIfBoundsChanged();
    virtual bool checkIfValueRangeChanged();

    virtual void dataChangedEvent();
    virtual void boundsChangedEvent();
    virtual void valueRangeChangedEvent();

private:
    void addObserver(const QString & eventName, vtkObject & subject, unsigned long tag);

private:
    DataObjectPrivate * d_ptr;
};

template<typename U, typename T>
void DataObject::connectObserver(const QString & eventName, vtkObject & subject, unsigned long event, U & observer, void(T::* slot)(void))
{
    addObserver(eventName, subject,
        subject.AddObserver(event, &observer, slot));
}

template<typename U, typename T>
void DataObject::connectObserver(const QString & eventName, vtkObject & subject, unsigned long event, U & observer, void(T::* slot)(vtkObject*, unsigned long, void*))
{
    addObserver(eventName, subject,
        subject.AddObserver(event, &observer, slot));
}
