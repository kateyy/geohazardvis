#pragma once

#include <memory>

#include <QObject>

#include <vtkObject.h>

#include <core/core_api.h>


class vtkInformation;
class vtkInformationStringKey;
class vtkInformationIntegerKey;
class vtkDataArray;
class vtkDataSet;
class vtkAlgorithmOutput;

class Context2DData;
class DataObjectPrivate;
class QVtkTableModel;
class RenderedData;


/** Base class representing loaded data. */
class CORE_API DataObject : public QObject
{
    Q_OBJECT

public:
    explicit DataObject(const QString & name, vtkDataSet * dataSet);
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

    vtkIdType numberOfPoints() const;
    vtkIdType numberOfCells() const;

    QVtkTableModel * tableModel();

    /** assign some kind of data array to my indexes.
        Let subclasses decide how to proceed with it; ignores this call by default */
    virtual void addDataArray(vtkDataArray & dataArray);

    /** Queue signals and events such as dataChanged(), boundsChanged() until executeDeferredEvents is called. 
      * This is required, when changing significant parts of the underlaying data set. 
      * E.g.: actor bounds updates might cause crash faults, if points or indices of a vtkPointSet are invalid. 
      * This method stacks, i.e., multiple subsequent calls of deferEvents require the same number of executeDeferredEvents calls
      * to actually execute the deferred events. 
      * @see ScopedEventDeferral */
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
    /** Emitted after changed of the geometrical structure, e.g., number of cells, points etc. */
    void structureChanged();

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
    virtual bool checkIfStructureChanged();

    virtual void dataChangedEvent();
    virtual void boundsChangedEvent();
    virtual void valueRangeChangedEvent();
    virtual void structureChangedEvent();

    bool deferringEvents() const;
    virtual void process_executeDeferredEvents();

private:
    void addObserver(const QString & eventName, vtkObject & subject, unsigned long tag);

private:
    std::unique_ptr<DataObjectPrivate> d_ptr;
};


/** Blocks the data object's events on constructions and requests execution on destruction.
This is most useful for an exception safe event lock on per-scope basis.
*/
class ScopedEventDeferral final
{
public:
    explicit ScopedEventDeferral(DataObject & objectToLock);
    ~ScopedEventDeferral();

    ScopedEventDeferral(const ScopedEventDeferral & other) = delete;
    ScopedEventDeferral & operator=(const ScopedEventDeferral & other) = delete;

private:
    DataObject & m_dataObject;
};


#include "DataObject.hpp"
