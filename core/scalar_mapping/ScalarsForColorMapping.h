#pragma once

#include <vector>

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>

#include <vtkType.h>

#include <core/core_api.h>


class vtkAlgorithm;
class vtkMapper;

class DataObject;


/**
Abstract base class for scalars that can be used for surface color mappings.
Scalar mappings extract relevant data from an input data objects and supply their defined scalar values.
A subrange of values may be used for the color mapping (setMinValue, setMaxValue).
*/
class CORE_API ScalarsForColorMapping : public QObject
{
    Q_OBJECT

public:
    friend class ScalarsForColorMappingRegistry;

    template<typename SubClass>
    static QList<ScalarsForColorMapping *> newInstance(const QList<DataObject*> & dataObjects);

public:
    explicit ScalarsForColorMapping(const QList<DataObject *> & dataObjects, vtkIdType numDataComponents = 1);
    virtual ~ScalarsForColorMapping();

    virtual QString name() const = 0;
    virtual QString scalarsName() const;

    vtkIdType numDataComponents() const;
    vtkIdType dataComponent() const;
    void setDataComponent(vtkIdType component);

    /** when mapping arrays that have more tuples than cells (triangles) exist in the data set:
        return highest possible index that can be used as first index in the scalar array. */
    virtual vtkIdType maximumStartingIndex();
    vtkIdType startingIndex() const;
    void setStartingIndex(vtkIdType index);

    QList<DataObject *> dataObjects() const;
    /** set a new order for the data objects. This list must contain exactly the pointers returned by dataObjects() */
    void rearrangeDataObjets(QList<DataObject *> dataObjects);

    /** create a filter to map values to color, applying current min/max settings
      * @param dataObject is required to setup object specific parameters on the filter.
      *        The filter inputs are set as required.
      * May be implemented by subclasses, returns nullptr by default. */
    virtual vtkAlgorithm * createFilter(DataObject * dataObject);
    virtual bool usesFilter() const;

    /** set parameters on the data object/data set and the mapper that is used to render the object.
        @param dataObject must be one of the objects that where passed when calling the mapping's constructor */
    virtual void configureDataObjectAndMapper(DataObject * dataObject, vtkMapper * mapper);

    /** minimal value in the data set 
        @param select a data component to query or use the currently selected component (-1) */
    double dataMinValue(vtkIdType component = -1) const;
    /** maximal value in the data set 
        @param select a data component to query or use the currently selected component (-1) */
    double dataMaxValue(vtkIdType component = -1) const;

    /** minimal value used for scalar mapping (clamped to [dataMinValue, dataMaxValue]) */
    double minValue(vtkIdType component = -1) const;
    void setMinValue(double value, vtkIdType component = -1);
    /** maximal value used for scalar mapping (clamped to [dataMinValue, dataMaxValue]) */
    double maxValue(vtkIdType component = -1) const;
    void setMaxValue(double value, vtkIdType component = -1);

signals:
    void minMaxChanged();
    void dataMinMaxChanged();

protected:
    virtual void initialize();
    virtual void updateBounds() = 0;

    /** check whether these scalar extraction is applicable for the data objects it was created with */
    virtual bool isValid() const = 0;

    void setDataMinMaxValue(const double minMax[2], vtkIdType component);
    void setDataMinMaxValue(double min, double max, vtkIdType component);

protected:
    virtual void dataComponentChangedEvent();
    virtual void minMaxChangedEvent();
    virtual void startingIndexChangedEvent();
    virtual void objectOrderChangedEvent();

protected:
    QList<DataObject *> m_dataObjects;

private:
    vtkIdType m_startingIndex;

    const vtkIdType m_numDataComponents;
    vtkIdType m_dataComponent;

    /** per data component: value range and user selected subrange */
    std::vector<double> m_dataMinValue;
    std::vector<double> m_dataMaxValue;
    std::vector<double> m_minValue;
    std::vector<double> m_maxValue;
};

#include "ScalarsForColorMapping.hpp"
