#include "ColorMappingData.h"

#include <cassert>
#include <limits>

#include <QSet>

#include <vtkMapper.h>
#include <vtkLookupTable.h>

#include <core/AbstractVisualizedData.h>
#include <core/data_objects/DataObject.h>
#include <core/utility/macros.h>


ColorMappingData::ColorMappingData(const QList<AbstractVisualizedData *> & visualizedData, int numDataComponents)
    : m_isValid(false)
    , m_visualizedData(visualizedData)
    , m_numDataComponents(numDataComponents)
    , m_dataComponent(0)
    , m_isActive(false)
    , m_dataMinValue(numDataComponents, std::numeric_limits<double>::max())
    , m_dataMaxValue(numDataComponents, std::numeric_limits<double>::lowest())
    , m_minValue(numDataComponents, std::numeric_limits<double>::max())
    , m_maxValue(numDataComponents, std::numeric_limits<double>::lowest())
    , m_boundsValid(false)
{
}

void ColorMappingData::activate()
{
    for (AbstractVisualizedData * vis : m_visualizedData)
        vis->setScalarsForColorMapping(this);

    if (m_lut)
    {
        m_lut->SetTableRange(minValue(), maxValue());
        m_lut->SetVectorModeToComponent();
        m_lut->SetVectorComponent(dataComponent());
        m_lut->Build();
    }

    m_isActive = true;
}

void ColorMappingData::deactivate()
{
    m_isActive = false;

    for (AbstractVisualizedData * vis : m_visualizedData)
        vis->setScalarsForColorMapping(nullptr);
}

bool ColorMappingData::isActive() const
{
    return m_isActive;
}

QString ColorMappingData::scalarsName() const
{
    return "";
}

int ColorMappingData::numDataComponents() const
{
    return m_numDataComponents;
}

int ColorMappingData::dataComponent() const
{
    return m_dataComponent;
}

void ColorMappingData::setDataComponent(int component)
{
    assert(0 <= component && component < m_numDataComponents);
    if (m_dataComponent == component)
        return;

    m_dataComponent = component;

    if (isActive() && m_lut)
        m_lut->SetVectorComponent(component);

    dataComponentChangedEvent();

    if (isActive() && m_lut)
    {
        m_lut->SetTableRange(minValue(), maxValue());
        m_lut->Build();
    }
}

void ColorMappingData::initialize()
{
    for (AbstractVisualizedData * vis : m_visualizedData)
    {
        connect(&vis->dataObject(), &DataObject::valueRangeChanged, this, &ColorMappingData::forceUpdateBoundsLocked);
    }
}

bool ColorMappingData::isValid() const
{
    return m_isValid;
}

vtkSmartPointer<vtkAlgorithm> ColorMappingData::createFilter(AbstractVisualizedData * /*visualizedData*/, int /*connection*/)
{
    return nullptr;
}

bool ColorMappingData::usesFilter() const
{
    return false;
}

void ColorMappingData::configureMapper(AbstractVisualizedData * DEBUG_ONLY(visualizedData), vtkAbstractMapper * /*mapper*/)
{
    assert(m_visualizedData.contains(visualizedData));
}

void ColorMappingData::setLookupTable(vtkLookupTable * lookupTable)
{
    if (m_lut == lookupTable)
        return;

    m_lut = lookupTable;

    lookupTableChangedEvent();
}

double ColorMappingData::dataMinValue(int component) const
{
    updateBoundsLocked();

    assert(m_dataMinValue <= m_dataMaxValue && component < m_numDataComponents);
    if (component < 0)
        component = m_dataComponent;
    return m_dataMinValue[component];
}

double ColorMappingData::dataMaxValue(int component) const
{
    updateBoundsLocked();

    assert(m_dataMinValue <= m_dataMaxValue && component < m_numDataComponents);
    if (component < 0)
        component = m_dataComponent;
    return m_dataMaxValue[component];
}

double ColorMappingData::minValue(int component) const
{
    updateBoundsLocked();

    if (component < 0)
        component = m_dataComponent;
    return m_minValue[component];
}

void ColorMappingData::setMinValue(double value, int component)
{
    updateBoundsLocked();

    if (component < 0)
        component = m_dataComponent;

    m_minValue[component] = std::min(std::max(m_dataMinValue[component], value), m_dataMaxValue[component]);

    minMaxChangedEvent();
}

double ColorMappingData::maxValue(int component) const
{
    updateBoundsLocked();

    if (component < 0)
        component = m_dataComponent;

    return m_maxValue[component];
}

void ColorMappingData::setMaxValue(double value, int component)
{
    updateBoundsLocked();

    if (component < 0)
        component = m_dataComponent;

    m_maxValue[component] = std::min(std::max(m_dataMinValue[component], value), m_dataMaxValue[component]);

    minMaxChangedEvent();
}

void ColorMappingData::lookupTableChangedEvent()
{
}

void ColorMappingData::dataComponentChangedEvent()
{
}

void ColorMappingData::minMaxChangedEvent()
{
    if (isActive() && m_lut)
    {
        m_lut->SetTableRange(minValue(), maxValue());
        m_lut->Build();
    }

    emit minMaxChanged();
}

void ColorMappingData::forceUpdateBoundsLocked() const
{
    const_cast<ColorMappingData *>(this)->m_boundsValid = false;

    updateBoundsLocked();
}

void ColorMappingData::updateBoundsLocked() const
{
    m_boundsUpdateMutex.lock();

    if (m_boundsValid)
    {
        m_boundsUpdateMutex.unlock();
        return;
    }

    auto & lockedThis = const_cast<ColorMappingData &>(*this);

    auto newBounds = lockedThis.updateBounds();

    bool minMaxChanged = false;

    for (auto it = newBounds.begin(); it != newBounds.end(); ++it)
    {
        auto component = it.key();
        const QPair<double, double> & minMax = it.value();

        assert(minMax.first <= minMax.second);

        if (lockedThis.m_dataMinValue[component] == minMax.first
            && lockedThis.m_dataMaxValue[component] == minMax.second)
        {
            continue;
        }

        minMaxChanged = true;

        lockedThis.m_minValue[component] = lockedThis.m_dataMinValue[component] = minMax.first;
        lockedThis.m_maxValue[component] = lockedThis.m_dataMaxValue[component] = minMax.second;
    }

    lockedThis.m_boundsValid = true;

    m_boundsUpdateMutex.unlock();

    if (minMaxChanged)
    {
        emit lockedThis.dataMinMaxChanged();
        lockedThis.minMaxChangedEvent();
    }
}
