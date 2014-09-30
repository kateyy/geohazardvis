#pragma once

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
    explicit ScalarsForColorMapping(const QList<DataObject *> & dataObjects);
    virtual ~ScalarsForColorMapping() = 0;

    virtual QString name() const = 0;

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

    /** minimal value in the data set */
    double dataMinValue() const;
    /** maximal value in the data set */
    double dataMaxValue() const;

    /** minimal value used for scalar mapping (clamped to [dataMinValue, dataMaxValue]) */
    double minValue() const;
    void setMinValue(double value);
    /** maximal value used for scalar mapping (clamped to [dataMinValue, dataMaxValue]) */
    double maxValue() const;
    void setMaxValue(double value);

signals:
    void minMaxChanged();
    void dataMinMaxChanged();

protected:
    virtual void initialize();
    virtual void updateBounds() = 0;

    /** check whether these scalar extraction is applicable for the data objects it was created with */
    virtual bool isValid() const = 0;

    void setDataMinMaxValue(const double minMax[2]);
    void setDataMinMaxValue(double min, double max);

protected:
    virtual void minMaxChangedEvent();
    virtual void startingIndexChangedEvent();
    virtual void objectOrderChangedEvent();

protected:
    QList<DataObject *> m_dataObjects;

private:
    vtkIdType m_startingIndex;

    double m_dataMinValue;
    double m_dataMaxValue;
    double m_minValue;
    double m_maxValue;
};

#include "ScalarsForColorMapping.hpp"
