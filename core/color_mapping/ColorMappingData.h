#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <QList>
#include <QObject>

#include <vtkSmartPointer.h>

#include <core/core_api.h>
#include <core/utility/DataExtent_fwd.h>


class QString;
class vtkAlgorithm;
class vtkAbstractMapper;
class vtkLookupTable;
class vtkScalarsToColors;

class AbstractVisualizedData;
enum class IndexType;


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
    static std::vector<std::unique_ptr<ColorMappingData>> newInstance(const QList<AbstractVisualizedData *> & visualizedData);

public:
    explicit ColorMappingData(const QList<AbstractVisualizedData *> & visualizedData,
        int numDataComponents = 1,
        bool mapsScalarsToColors = true,
        bool usesOwnLookupTable = false);

    virtual void activate();
    virtual void deactivate();
    /** @return true only if this object is active. Initially, an object is inactive.
    * Only active objects shall change the state of depending object (e.g, the lookup table) */
    bool isActive() const;

    virtual QString name() const = 0;
    /** Name of the scalar/vector array that is mapped to colors. This can be empty.
    * Also, this can be an array that is part of the source data object, or an additional attribute
    * created by the color mapping itself. */
    virtual QString scalarsName(AbstractVisualizedData & vis) const;
    virtual IndexType scalarsAssociation(AbstractVisualizedData & vis) const;

    int numDataComponents() const;
    int dataComponent() const;
    void setDataComponent(int component);

    /** Return whether scalars are mapped/transformed to colors or used directly as color values. */
    bool mapsScalarsToColors() const;

    /** Return whether this mapping requires an own lookup table instead of a user-configured one.
      * If this returns true, use the lookup table returned by ownLookupTable() in visualizations. */
    bool usesOwnLookupTable() const;
    vtkScalarsToColors * ownLookupTable();

    /** create a filter to map values to color, applying current min/max settings
      * @param visualizedData is required to setup object specific parameters on the filter.
      *        The filter inputs are set as required.
      * May be implemented by subclasses, returns nullptr by default. */
    virtual vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData & visualizedData, int connection = 0);
    virtual bool usesFilter() const;

    /** set parameters on the mapper that is used to render the visualizedData.
        @param visualizedData must be one of the objects that where passed when calling the mapping's constructor */
    virtual void configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper);

    void setLookupTable(vtkLookupTable * lookupTable);

    /** minimal value in the data set
        @param select a data component to query or use the currently selected component (-1) */
    double dataMinValue(int component = -1) const;
    /** maximal value in the data set
        @param select a data component to query or use the currently selected component (-1) */
    double dataMaxValue(int component = -1) const;

    /** minimal value used for color mapping (clamped to [dataMinValue, dataMaxValue]) */
    double minValue(int component = -1) const;
    void setMinValue(double value, int component = -1);
    /** maximal value used for color mapping (clamped to [dataMinValue, dataMaxValue]) */
    double maxValue(int component = -1) const;
    void setMaxValue(double value, int component = -1);

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
        @return a vector containing value ranges per component */
    virtual std::vector<ValueRange<>> updateBounds() = 0;

    /** Subclass have to override this if and only if usesOwnLookupTable was set to true in the constructor. */
    virtual vtkSmartPointer<vtkScalarsToColors> createOwnLookupTable();

protected:
    virtual void lookupTableChangedEvent();
    virtual void dataComponentChangedEvent();
    virtual void minMaxChangedEvent();

protected:
    bool m_isValid;
    QList<AbstractVisualizedData *> m_visualizedData;
    vtkSmartPointer<vtkLookupTable> m_lut;
    vtkSmartPointer<vtkScalarsToColors> m_ownLut;

private:
    void forceUpdateBoundsLocked() const;
    void updateBoundsLocked() const;

private:
    const int m_numDataComponents;
    int m_dataComponent;
    bool m_isActive;

    const bool m_mapsScalarsToColors;
    const bool m_usesOwnLookupTable;

    /** per data component: value range and user selected subrange */
    std::vector<double> m_dataMinValue;
    std::vector<double> m_dataMaxValue;
    std::vector<double> m_minValue;
    std::vector<double> m_maxValue;
    bool m_boundsValid;
    mutable std::mutex m_boundsUpdateMutex;
};

#include "ColorMappingData.hpp"
