#pragma once

#include <vector>

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkAlgorithm;
class vtkMapper;
class vtkLookupTable;

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

    /** Call this before using these scalars for rendering.
      * Sets up the lookup table for the current scene. */
    void beforeRendering();

    virtual QString name() const = 0;
    virtual QString scalarsName() const;

    vtkIdType numDataComponents() const;
    vtkIdType dataComponent() const;
    void setDataComponent(vtkIdType component);

    /** create a filter to map values to color, applying current min/max settings
      * @param dataObject is required to setup object specific parameters on the filter.
      *        The filter inputs are set as required.
      * May be implemented by subclasses, returns nullptr by default. */
    virtual vtkAlgorithm * createFilter(DataObject * dataObject);
    virtual bool usesFilter() const;

    /** set parameters on the data object/data set and the mapper that is used to render the object.
        @param dataObject must be one of the objects that where passed when calling the mapping's constructor */
    virtual void configureDataObjectAndMapper(DataObject * dataObject, vtkMapper * mapper);

    void setLookupTable(vtkLookupTable * lookupTable);

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
    void lookupTableChanged();
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
    virtual void beforeRenderingEvent();
    virtual void lookupTableChangedEvent();
    virtual void dataComponentChangedEvent();
    virtual void minMaxChangedEvent();

protected:
    QList<DataObject *> m_dataObjects;
    vtkSmartPointer<vtkLookupTable> m_lut;

private:
    const vtkIdType m_numDataComponents;
    vtkIdType m_dataComponent;

    /** per data component: value range and user selected subrange */
    std::vector<double> m_dataMinValue;
    std::vector<double> m_dataMaxValue;
    std::vector<double> m_minValue;
    std::vector<double> m_maxValue;
};

#include "ScalarsForColorMapping.hpp"
