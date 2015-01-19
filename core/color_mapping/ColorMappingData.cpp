#include "ColorMappingData.h"

#include <cassert>
#include <limits>

#include <vtkMapper.h>
#include <vtkLookupTable.h>

#include <QSet>

#include <core/AbstractVisualizedData.h>
#include <core/data_objects/DataObject.h>


ColorMappingData::ColorMappingData(const QList<AbstractVisualizedData *> & visualizedData, vtkIdType numDataComponents)
    : m_isValid(false)
    , m_visualizedData(visualizedData)
    , m_numDataComponents(numDataComponents)
    , m_dataComponent(0)
    , m_dataMinValue(numDataComponents, std::numeric_limits<double>::max())
    , m_dataMaxValue(numDataComponents, std::numeric_limits<double>::lowest())
    , m_minValue(numDataComponents, std::numeric_limits<double>::max())
    , m_maxValue(numDataComponents, std::numeric_limits<double>::lowest())
{
}

ColorMappingData::~ColorMappingData() = default;

void ColorMappingData::beforeRendering()
{
    if (m_lut)
    {
        m_lut->SetTableRange(minValue(), maxValue());
    }

    beforeRenderingEvent();
}

QString ColorMappingData::scalarsName() const
{
    return "";
}

vtkIdType ColorMappingData::numDataComponents() const
{
    return m_numDataComponents;
}

vtkIdType ColorMappingData::dataComponent() const
{
    return m_dataComponent;
}

void ColorMappingData::setDataComponent(vtkIdType component)
{
    assert(0 <= component && component < m_numDataComponents);
    if (m_dataComponent == component)
        return;

    m_dataComponent = component;

    if (m_lut)
        m_lut->SetVectorComponent(component);

    dataComponentChangedEvent();

    if (m_lut)
        m_lut->SetTableRange(minValue(), maxValue());
}

void ColorMappingData::initialize()
{
    for (AbstractVisualizedData * vis : m_visualizedData)
    {
        DataObject * dataObject = vis->dataObject();
        assert(dataObject);
        connect(dataObject, &DataObject::valueRangeChanged, this, &ColorMappingData::updateBounds);
    }

    updateBounds();
}

bool ColorMappingData::isValid() const
{
    return m_isValid;
}

vtkAlgorithm * ColorMappingData::createFilter(AbstractVisualizedData * /*visualizedData*/)
{
    return nullptr;
}

bool ColorMappingData::usesFilter() const
{
    return false;
}

void ColorMappingData::configureMapper(AbstractVisualizedData * /*visualizedData*/, vtkMapper * /*mapper*/)
{
}

void ColorMappingData::setLookupTable(vtkLookupTable * lookupTable)
{
    if (m_lut == lookupTable)
        return;

    m_lut = lookupTable;

    lookupTableChangedEvent();
}

double ColorMappingData::dataMinValue(vtkIdType component) const
{
    assert(m_dataMinValue <= m_dataMaxValue && component < m_numDataComponents);
    if (component < 0)
        component = m_dataComponent;
    return m_dataMinValue[component];
}

double ColorMappingData::dataMaxValue(vtkIdType component) const
{
    assert(m_dataMinValue <= m_dataMaxValue && component < m_numDataComponents);
    if (component < 0)
        component = m_dataComponent;
    return m_dataMaxValue[component];
}

double ColorMappingData::minValue(vtkIdType component) const
{
    if (component < 0)
        component = m_dataComponent;
    return m_minValue[component];
}

void ColorMappingData::setMinValue(double value, vtkIdType component)
{
    if (component < 0)
        component = m_dataComponent;

    m_minValue[component] = std::min(std::max(m_dataMinValue[component], value), m_dataMaxValue[component]);

    minMaxChangedEvent();
}

double ColorMappingData::maxValue(vtkIdType component) const
{
    if (component < 0)
        component = m_dataComponent;

    return m_maxValue[component];
}

void ColorMappingData::setMaxValue(double value, vtkIdType component)
{
    if (component < 0)
        component = m_dataComponent;

    m_maxValue[component] = std::min(std::max(m_dataMinValue[component], value), m_dataMaxValue[component]);

    minMaxChangedEvent();
}

void ColorMappingData::setDataMinMaxValue(const double minMax[2], vtkIdType component)
{
    if (component < 0)
        component = m_dataComponent;

    m_dataMinValue[component] = minMax[0];
    m_dataMaxValue[component] = minMax[1];

    bool minMaxChanged = false;

    // reset user selected ranges only if really needed
    if (m_minValue[component] < m_dataMinValue[component] || m_minValue[component] > m_dataMaxValue[component])
    {
        m_minValue[component] = m_dataMinValue[component];
        minMaxChanged = true;
    }
    if (m_maxValue[component] < m_dataMinValue[component] || m_maxValue[component] > m_dataMaxValue[component])
    {
        m_maxValue[component] = m_dataMaxValue[component];
        minMaxChanged = true;
    }

    if (minMaxChanged)
        minMaxChangedEvent();

    emit dataMinMaxChanged();
}

void ColorMappingData::setDataMinMaxValue(double min, double max, vtkIdType component)
{
    double minMax[2] = { min, max };
    setDataMinMaxValue(minMax, component);
}

void ColorMappingData::beforeRenderingEvent()
{
    if (m_lut)
    {
        m_lut->SetVectorModeToComponent();
        m_lut->SetVectorComponent(dataComponent());
    }
}

void ColorMappingData::lookupTableChangedEvent()
{
}

void ColorMappingData::dataComponentChangedEvent()
{
}

void ColorMappingData::minMaxChangedEvent()
{
    if (m_lut)
        m_lut->SetTableRange(minValue(), maxValue());

    emit minMaxChanged();
}
