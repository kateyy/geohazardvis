#pragma once

#include <memory>

#include <QObject>

#include <vtkObject.h>

#include <core/core_api.h>
#include <core/table_model/QVtkTableModel.h>


class vtkInformation;
class vtkInformationStringKey;
class vtkInformationIntegerKey;
class vtkDataArray;
class vtkDataSet;
class vtkAlgorithmOutput;
class RenderedData;
class Context2DData;

class DataObjectPrivate;


/** Base class representing loaded data. */
class CORE_API DataObject : public QObject
{
    Q_OBJECT

public:
    DataObject(const QString & name, vtkDataSet * dataSet);
    ~DataObject() override;

    /** @return true if this is a 3D geometry (and false if it's image/2D data) */
    virtual bool is3D() const = 0;

    /** create a visualization instance; Subclasses may implement one or both
        -> createRendered: flexible/generic rendering 
        -> createContextData: for specialized views (e.g., plots)
    */
    virtual std::unique_ptr<RenderedData> createRendered();
    virtual std::unique_ptr<Context2DData> createContextData();

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
    virtual void addDataArray(vtkDataArray & dataArray);

    /** Queue signals and events such as dataChanged(), boundsChanged() until executeDeferredEvents is called. 
      * This is required, when changing significant parts of the underlaying data set. 
      * E.g.: actor bounds updates might cause crash faults, if points or indices of a vtkPointSet are invalid. 
      * This method stacks, i.e., multiple subsequent calls of deferEvents require the same number of executeDeferredEvents calls
      * to actually execute the deferred events. */
    void deferEvents();
    void executeDeferredEvents();

    static vtkInformationStringKey * NameKey();
    static vtkInformationIntegerKey * ArrayIsAuxiliaryKey();

    static DataObject * readPointer(vtkInformation & information);
    static void storePointer(vtkInformation & information, DataObject * dataObject);

    DataObject(const DataObject &) = delete;
    DataObject(DataObject &&) = delete;
    void operator=(const DataObject &) = delete;
    void operator=(DataObject &&) = delete;

signals:
    void dataChanged();
    void boundsChanged();
    void valueRangeChanged();
    void attributeArraysChanged();

protected:
    virtual std::unique_ptr<QVtkTableModel> createTableModel() = 0;

    template<typename U, typename T>
    void connectObserver(const QString & eventName, vtkObject & subject,
        unsigned long event, U & observer, void(T::* slot)(void));
    template<typename U, typename T>
    void connectObserver(const QString & eventName, vtkObject & subject,
        unsigned long event, U & observer, void(T::* slot)(vtkObject*, unsigned long, void*));
    void disconnectEventGroup(const QString & eventName);
    void disconnectAllEvents();

    void _dataChanged();
    void _attributeArraysChanged();

protected:
    /** when data set values changed, check whether this also affects the bounds*/
    virtual bool checkIfBoundsChanged();
    virtual bool checkIfValueRangeChanged();

    virtual void dataChangedEvent();
    virtual void boundsChangedEvent();
    virtual void valueRangeChangedEvent();

    bool deferringEvents() const;
    virtual void process_executeDeferredEvents();

private:
    void addObserver(const QString & eventName, vtkObject & subject, unsigned long tag);

private:
    std::unique_ptr<DataObjectPrivate> d_ptr;
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
