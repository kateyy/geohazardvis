#pragma once

#include <mutex>
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

class AbstractVisualizedData;


/**
Abstract base class for scalar data that can be used for surface color mappings.
Color mappings extract relevant data from input data objects and supply their defined scalar values.
A subrange of values may be used for the color mapping (setMinValue, setMaxValue).
*/
class CORE_API ColorMappingData : public QObject
{
    Q_OBJECT

public:
    friend class ColorMappingRegistry;

    template<typename SubClass>
    static QList<ColorMappingData *> newInstance(const QList<AbstractVisualizedData *> & visualizedData);

public:
    explicit ColorMappingData(const QList<AbstractVisualizedData *> & visualizedData, vtkIdType numDataComponents = 1);
    virtual ~ColorMappingData() override;

    virtual void activate();
    virtual void deactivate();

    virtual QString name() const = 0;
    virtual QString scalarsName() const;

    vtkIdType numDataComponents() const;
    vtkIdType dataComponent() const;
    void setDataComponent(vtkIdType component);

    /** create a filter to map values to color, applying current min/max settings
      * @param dataObject is required to setup object specific parameters on the filter.
      *        The filter inputs are set as required.
      * May be implemented by subclasses, returns nullptr by default. */
    virtual vtkAlgorithm * createFilter(AbstractVisualizedData * visualizedData);
    virtual bool usesFilter() const;

    /** set parameters on the mapper that is used to render the visualizedData.
        @param visualizedData must be one of the objects that where passed when calling the mapping's constructor */
    virtual void configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper);

    void setLookupTable(vtkLookupTable * lookupTable);

    /** minimal value in the data set 
        @param select a data component to query or use the currently selected component (-1) */
    double dataMinValue(vtkIdType component = -1) const;
    /** maximal value in the data set 
        @param select a data component to query or use the currently selected component (-1) */
    double dataMaxValue(vtkIdType component = -1) const;

    /** minimal value used for color mapping (clamped to [dataMinValue, dataMaxValue]) */
    double minValue(vtkIdType component = -1) const;
    void setMinValue(double value, vtkIdType component = -1);
    /** maximal value used for color mapping (clamped to [dataMinValue, dataMaxValue]) */
    double maxValue(vtkIdType component = -1) const;
    void setMaxValue(double value, vtkIdType component = -1);

signals:
    void lookupTableChanged();
    void minMaxChanged();
    void dataMinMaxChanged();

protected:
    /** check whether these scalar extraction is applicable for the data objects it was created with */
    bool isValid() const;
    /** call initialize on valid instances */
    virtual void initialize();

    /** update data bounds
        @return a map of component -> (min, max) */
    virtual QMap<vtkIdType, QPair<double, double>> updateBounds() = 0;

protected:
    virtual void lookupTableChangedEvent();
    virtual void dataComponentChangedEvent();
    virtual void minMaxChangedEvent();

protected:
    bool m_isValid;
    QList<AbstractVisualizedData *> m_visualizedData;
    vtkSmartPointer<vtkLookupTable> m_lut;

private:
    void updateBoundsLocked() const;

private:
    const vtkIdType m_numDataComponents;
    vtkIdType m_dataComponent;

    /** per data component: value range and user selected subrange */
    std::vector<double> m_dataMinValue;
    std::vector<double> m_dataMaxValue;
    std::vector<double> m_minValue;
    std::vector<double> m_maxValue;
    bool m_boundsValid;
    mutable std::mutex m_boundsUpdateMutex;
};

#include "ColorMappingData.hpp"
