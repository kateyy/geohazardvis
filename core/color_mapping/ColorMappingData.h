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
#include <mutex>
#include <vector>

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
 * Abstract base class for scalar data that can be used for surface color mappings.
 *
 * Color mappings extract relevant data from input data objects and supply their defined scalar values.
 * A subrange of values may be used for the color mapping (setMinValue, setMaxValue).
*/
class CORE_API ColorMappingData : public QObject
{
    Q_OBJECT

public:
    friend class ColorMappingRegistry;

public:
    explicit ColorMappingData(const std::vector<AbstractVisualizedData *> & visualizedData,
        int numDataComponents = 1,
        bool mapsScalarsToColors = true,
        bool usesOwnLookupTable = false);
    ~ColorMappingData() override;

    void activate();
    void deactivate();
    /** @return true only if this object is active. Initially, an object is inactive.
     * Only active objects shall change the state of depending object (e.g, the lookup table) */
    bool isActive() const;

    virtual QString name() const = 0;
    /** Name of the scalar/vector array that is mapped to colors. This can be empty.
     * Also, this can be an array that is part of the source data object, or an additional attribute
     * created by the color mapping itself. */
    virtual QString scalarsName(AbstractVisualizedData & vis) const;
    virtual IndexType scalarsAssociation(AbstractVisualizedData & vis) const;

    /** Check if this attribute has time steps.
     * This function returns false in this base class. ColorMappingData generally does not take
     * take care of time steps, this must be done in the upstream pipeline. This function only
     * simplifies to detect temporal data in the current setup. */
    virtual bool isTemporalAttribute() const;

    int numDataComponents() const;
    virtual QString componentName(int component) const;
    int dataComponent() const;
    void setDataComponent(int component);

    /** Return whether scalars are mapped/transformed to colors or used directly as color values. */
    bool mapsScalarsToColors() const;

    /** Return whether this mapping requires an own lookup table instead of a user-configured one.
     * If this returns true, use the lookup table returned by ownLookupTable() in visualizations. */
    bool usesOwnLookupTable() const;
    vtkScalarsToColors * ownLookupTable();

    /** Create a filter to map values to color, applying current min/max settings.
     * @param visualizedData is required to setup object specific parameters on the filter.
     *        The filter inputs are set as required.
     * May be implemented by subclasses, returns nullptr by default. */
    virtual vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData & visualizedData, unsigned int port = 0);
    virtual bool usesFilter() const;

    /** Set parameters on the mapper that is used to render the visualizedData.
     * @param visualizedData must be one of the objects that where passed when calling the mapping's constructor */
    virtual void configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper, unsigned int port = 0);

    void setLookupTable(vtkLookupTable * lookupTable);

    /** Minimal value in the data set
     * @param select a data component to query or use the currently selected component (-1) */
    double dataMinValue(int component = -1) const;
    /** Maximal value in the data set
     * @param select a data component to query or use the currently selected component (-1) */
    double dataMaxValue(int component = -1) const;

    /** Minimal value used for color mapping (clamped to [dataMinValue, dataMaxValue]) */
    double minValue(int component = -1) const;
    void setMinValue(double value, int component = -1);
    /** Maximal value used for color mapping (clamped to [dataMinValue, dataMaxValue]) */
    double maxValue(int component = -1) const;
    void setMaxValue(double value, int component = -1);

signals:
    void lookupTableChanged();
    void minMaxChanged();
    void dataMinMaxChanged();
    void componentChanged();

protected:
    /** check whether these scalar extraction is applicable for the data objects it was created with */
    bool isValid() const;
    /** call initialize on valid instances */
    virtual void initialize();

    virtual void onActivate();
    virtual void onDeactivate();
    virtual void assignToVisualization();
    virtual void unassignFromVisualization();

    /** Update data bounds
     * @return a vector containing value ranges per component */
    virtual std::vector<ValueRange<>> updateBounds() = 0;

    /** Subclass have to override this if and only if usesOwnLookupTable was set to true in the constructor. */
    virtual vtkSmartPointer<vtkScalarsToColors> createOwnLookupTable();

protected:
    virtual void lookupTableChangedEvent();
    virtual void dataComponentChangedEvent();
    virtual void minMaxChangedEvent();

    void forceUpdateBoundsLocked() const;
    void updateBoundsLocked() const;

protected:
    bool m_isValid;
    std::vector<AbstractVisualizedData *> m_visualizedData;
    vtkSmartPointer<vtkLookupTable> m_lut;
    vtkSmartPointer<vtkScalarsToColors> m_ownLut;

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

private:
    Q_DISABLE_COPY(ColorMappingData)
};
