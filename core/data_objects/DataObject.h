/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>

#include <QObject>
#include <vtkSmartPointer.h>

#include <vtkObject.h>

#include <core/core_api.h>
#include <core/utility/DataExtent_fwd.h>


class QStringList;
class vtkAlgorithm;
class vtkAlgorithmOutput;
class vtkDataArray;
class vtkDataSet;
class vtkInformation;
class vtkInformationIntegerKey;

class Context2DData;
class DataObjectPrivate;
enum class IndexType;
class QVtkTableModel;
class RenderedData;
class ScopedEventDeferral;


/** Base class for data set representations. */
class CORE_API DataObject : public QObject
{
    Q_OBJECT

public:
    explicit DataObject(const QString & name, vtkDataSet * dataSet);
    ~DataObject() override;

    /**
     * Based on a concrete DataObject instance, create a new instance of the same class.
     *
     * Not all subclasses implement this function, as some constructor parameters are incompatible.
     * In this case, nullptr shall be returned.
     */
    virtual std::unique_ptr<DataObject> newInstance(const QString & name, vtkDataSet * dataSet) const = 0;

    /** @return true if this is a 3D geometry (and false if it's image/2D data) */
    virtual bool is3D() const = 0;
    virtual IndexType defaultAttributeLocation() const = 0;

    /**
     * Create a visualization instance. Subclasses may implement one or both visualization styles.
     *
     *  - createRendered: flexible/generic rendering
     *  - createContextData: for specialized views (e.g., plots)
    */
    virtual std::unique_ptr<RenderedData> createRendered();
    virtual std::unique_ptr<Context2DData> createContextData();

    const QString & name() const;
    virtual const QString & dataTypeName() const = 0;

    vtkDataSet * dataSet();
    const vtkDataSet * dataSet() const;

    /**
     * Copy structure of other to this->dataSet() (if is set).
     * This is a workaround for vtkPolyData::CopyStructure that does not trigger Modified events
     * for modifications of cell arrays.
     * This method should always be used instead of using vtkDataSet::CopyStructure directly!
     */
    void CopyStructure(vtkDataSet & other);

    /**
     * @return the source data set with specific modifications or enhancements, e.g., computed
     * normals
     */
    vtkAlgorithmOutput * processedOutputPort();
    /**
     * Convenience method that returns a persistent shallow copy of the output data set of
     * processedOutputPort()
     */
    vtkDataSet * processedOutputDataSet();

    /**
     * Allow to inject additional post processing steps to the pipeline without directly modifying
     * the AbstractVisualizedData class hierarchy.
     */
    struct PostProcessingStep
    {
        /** Processing step entry point that is connected to the current endpoint of the pipeline. */
        vtkSmartPointer<vtkAlgorithm> pipelineHead;
        /** Last step of processing step that will be the new endpoint of the pipeline. */
        vtkSmartPointer<vtkAlgorithm> pipelineTail;
    };
    /**
     * Add a post processing step to the processing pipeline.
     * @return A boolean that is true if the step was successfully added, and an ID that can later
     *  be used to erase the processing step from the pipeline.
     */
    std::pair<bool, unsigned int> injectPostProcessingStep(const PostProcessingStep & postProcessingStep);
    bool erasePostProcessingStep(unsigned int id);

    /** @return Cached versions of the spatial bounds of the source data set */
    const DataBounds & bounds() const;

    vtkIdType numberOfPoints() const;
    vtkIdType numberOfCells() const;

    QVtkTableModel * tableModel();

    /**
     * Add a data array to the data object's default attribute location (i.e., point or cell data).
     * This must be implemented by subclasses in order to have an effect.
     */
    virtual void addDataArray(vtkDataArray & dataArray);
    /**
     * Remove all attributes for point, cell and field data, except those that are required for the
     * DataObject itself.
     *
     * Remaining attributes are: field data "Name"
     */
    void clearAttributes();

    /**
     * Queue signals and events such as dataChanged(), boundsChanged() until executeDeferredEvents
     * is called.
     *
     * This is required, when changing significant parts of the underlaying data set.
     * E.g.: Actor bounds updates might cause crash faults, if points or indices of a vtkPointSet
     * are invalid.
     * This method stacks, i.e., multiple subsequent calls of deferEvents require the same number
     * of executeDeferredEvents calls to actually execute the deferred events.
     * @see ScopedEventDeferral
     */
    void deferEvents();
    void executeDeferredEvents();
    bool isDeferringEvents() const;
    ScopedEventDeferral scopedEventDeferral();

    /** Mark attribute arrays that should not be visible to the user, e.g., in color or glyph mappings. */
    static vtkInformationIntegerKey * ARRAY_IS_AUXILIARY();

    static DataObject * readPointer(vtkInformation & information);
    static void storePointer(vtkInformation & information, DataObject * dataObject);

    static QString readName(vtkInformation & information);
    static void storeName(vtkInformation & information, const DataObject & dataObject);


    /**
     * Inform other components of modifications in the data set.
     * The corresponding signals will be emitted once the DataObject is not anymore deferring its 
     * events.
     */
    void signal_dataChanged();
    void signal_attributeArraysChanged();

signals:
    void dataChanged();
    void boundsChanged();
    void valueRangeChanged();
    void attributeArraysChanged();
    /** Emitted after changed of the geometrical structure, e.g., number of cells, points etc. */
    void structureChanged();

protected:
    friend class DataObjectPrivate;
    DataObjectPrivate & dPtr();

    /**
     * Allows subclasses to statically customize the processing pipeline.
     * Default output is a vtkTrivialProducer containing this->dataSet().
     */
    virtual vtkAlgorithmOutput * processedOutputPortInternal();

    virtual std::unique_ptr<QVtkTableModel> createTableModel() = 0;

    template<typename U, typename T>
    void connectObserver(const QString & eventName, vtkObject & subject,
        unsigned long event, U & observer, void(T::* slot)(void));
    template<typename U, typename T>
    void connectObserver(const QString & eventName, vtkObject & subject,
        unsigned long event, U & observer, void(T::* slot)(vtkObject*, unsigned long, void*));
    void disconnectEventGroup(const QString & eventName);
    void disconnectAllEvents();

protected:
    /** Implement in subclasses to add attributes that should not be removed in clearAttributes() */
    virtual void addIntrinsicAttributes(
        QStringList & fieldAttributes,
        QStringList & pointAttributes,
        QStringList & cellAttributes);

    /** When data set values changed, check whether this also affects the bounds. */
    virtual bool checkIfBoundsChanged();
    virtual bool checkIfValueRangeChanged();
    virtual bool checkIfStructureChanged();

    virtual void dataChangedEvent();
    virtual void boundsChangedEvent();
    virtual void valueRangeChangedEvent();
    virtual void structureChangedEvent();

private:
    void addObserver(const QString & eventName, vtkObject & subject, unsigned long tag);

private:
    std::unique_ptr<DataObjectPrivate> d_ptr;

private:
    Q_DISABLE_COPY(DataObject)
};


/**
 * RAII style blocking of data object's signals.
 *
 * This class calls deferEvents() on construction and executeDeferredEvents() on destruction on the
 * specified data object.
*/
class ScopedEventDeferral final
{
public:
    explicit ScopedEventDeferral(DataObject & objectToLock);
    explicit ScopedEventDeferral(DataObject * objectToLock = nullptr);
    ~ScopedEventDeferral();

    DataObject * lockedObject();

    ScopedEventDeferral(ScopedEventDeferral && other);
    ScopedEventDeferral & operator=(ScopedEventDeferral && other);

    ScopedEventDeferral(const ScopedEventDeferral & other) = delete;
    ScopedEventDeferral & operator=(const ScopedEventDeferral & other) = delete;

private:
    DataObject * m_dataObject;
};


#include "DataObject.hpp"
