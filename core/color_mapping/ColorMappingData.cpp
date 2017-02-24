#include "ColorMappingData.h"

#include <algorithm>
#include <cassert>

#include <vtkAlgorithm.h>
#include <vtkLookupTable.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <core/data_objects/DataObject.h>
#include <core/utility/DataExtent.h>
#include <core/utility/macros.h>


ColorMappingData::ColorMappingData(const std::vector<AbstractVisualizedData *> & visualizedData,
    int numDataComponents,
    bool mapsScalarsToColors,
    bool usesOwnLookupTable)
    : m_isValid{ false }
    , m_visualizedData{ visualizedData }
    , m_numDataComponents{ numDataComponents }
    , m_dataComponent{ 0 }
    , m_isActive{ false }
    , m_mapsScalarsToColors{ mapsScalarsToColors }
    , m_usesOwnLookupTable{ usesOwnLookupTable }
    , m_dataMinValue(numDataComponents, std::numeric_limits<double>::max())
    , m_dataMaxValue(numDataComponents, std::numeric_limits<double>::lowest())
    , m_minValue(numDataComponents, std::numeric_limits<double>::max())
    , m_maxValue(numDataComponents, std::numeric_limits<double>::lowest())
    , m_boundsValid{ false }
{
    for (auto vis : visualizedData)
    {
        connect(vis, &AbstractVisualizedData::geometryChanged, this, &ColorMappingData::forceUpdateBoundsLocked);
    }
}

ColorMappingData::~ColorMappingData() = default;

void ColorMappingData::activate()
{
    assignToVisualization();

    if (m_lut)
    {
        m_lut->SetTableRange(minValue(), maxValue());
        m_lut->SetVectorModeToComponent();
        m_lut->SetVectorComponent(dataComponent());
        m_lut->Build();
    }

    m_isActive = true;

    onActivate();
}

void ColorMappingData::deactivate()
{
    m_isActive = false;

    onDeactivate();

    unassignFromVisualization();
}

bool ColorMappingData::isActive() const
{
    return m_isActive;
}

QString ColorMappingData::scalarsName(AbstractVisualizedData & /*vis*/) const
{
    return "";
}

IndexType ColorMappingData::scalarsAssociation(AbstractVisualizedData & /*vis*/) const
{
    return IndexType::invalid;
}

bool ColorMappingData::isTemporalAttribute() const
{
    return false;
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
    {
        return;
    }

    m_dataComponent = component;

    if (isActive() && m_lut)
    {
        m_lut->SetVectorComponent(component);
    }

    dataComponentChangedEvent();

    if (isActive() && m_lut)
    {
        m_lut->SetTableRange(minValue(), maxValue());
        m_lut->Build();
    }

    emit componentChanged();
}

bool ColorMappingData::mapsScalarsToColors() const
{
    return m_mapsScalarsToColors;
}

bool ColorMappingData::usesOwnLookupTable() const
{
    return m_usesOwnLookupTable;
}

vtkScalarsToColors * ColorMappingData::ownLookupTable()
{
    assert(usesOwnLookupTable());

    if (!m_ownLut)
    {
        m_ownLut = createOwnLookupTable();
    }

    return m_ownLut;
}

void ColorMappingData::initialize()
{
    for (auto vis : m_visualizedData)
    {
        connect(&vis->dataObject(), &DataObject::valueRangeChanged, this, &ColorMappingData::forceUpdateBoundsLocked);
    }
}

void ColorMappingData::onActivate()
{
}

void ColorMappingData::onDeactivate()
{
}

void ColorMappingData::assignToVisualization()
{
    for (auto vis : m_visualizedData)
    {
        vis->setScalarsForColorMapping(this);
    }
}

void ColorMappingData::unassignFromVisualization()
{

    for (auto vis : m_visualizedData)
    {
        vis->setScalarsForColorMapping(nullptr);
    }
}

bool ColorMappingData::isValid() const
{
    return m_isValid;
}

vtkSmartPointer<vtkAlgorithm> ColorMappingData::createFilter(
    AbstractVisualizedData & /*visualizedData*/,
    unsigned int /*port*/)
{
    return nullptr;
}

bool ColorMappingData::usesFilter() const
{
    return false;
}

void ColorMappingData::configureMapper(
    AbstractVisualizedData & DEBUG_ONLY(visualizedData),
    vtkAbstractMapper & /*mapper*/,
    unsigned int /*port*/)
{
    assert(std::find(m_visualizedData.begin(), m_visualizedData.end(), &visualizedData) != m_visualizedData.end());
}

void ColorMappingData::setLookupTable(vtkLookupTable * lookupTable)
{
    if (m_lut == lookupTable)
    {
        return;
    }

    m_lut = lookupTable;

    lookupTableChangedEvent();
}

double ColorMappingData::dataMinValue(int component) const
{
    updateBoundsLocked();

    assert(m_dataMinValue <= m_dataMaxValue && component < m_numDataComponents);
    if (component < 0)
    {
        component = m_dataComponent;
    }
    return m_dataMinValue[component];
}

double ColorMappingData::dataMaxValue(int component) const
{
    updateBoundsLocked();

    assert(m_dataMinValue <= m_dataMaxValue && component < m_numDataComponents);
    if (component < 0)
    {
        component = m_dataComponent;
    }
    return m_dataMaxValue[component];
}

double ColorMappingData::minValue(int component) const
{
    updateBoundsLocked();

    if (component < 0)
    {
        component = m_dataComponent;
    }
    return m_minValue[component];
}

void ColorMappingData::setMinValue(double value, int component)
{
    updateBoundsLocked();

    if (component < 0)
    {
        component = m_dataComponent;
    }

    m_minValue[component] = std::min(std::max(m_dataMinValue[component], value), m_dataMaxValue[component]);

    minMaxChangedEvent();
}

double ColorMappingData::maxValue(int component) const
{
    updateBoundsLocked();

    if (component < 0)
    {
        component = m_dataComponent;
    }

    return m_maxValue[component];
}

void ColorMappingData::setMaxValue(double value, int component)
{
    updateBoundsLocked();

    if (component < 0)
    {
        component = m_dataComponent;
    }

    m_maxValue[component] = std::min(std::max(m_dataMinValue[component], value), m_dataMaxValue[component]);

    minMaxChangedEvent();
}

vtkSmartPointer<vtkScalarsToColors> ColorMappingData::createOwnLookupTable()
{
    assert(false);
    return{};
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
    assert(newBounds.size() == static_cast<size_t>(m_numDataComponents));

    bool minMaxChanged = false;

    for (int component = 0; component < m_numDataComponents; ++component)
    {
        const auto & valueRange = newBounds[component];

        assert(!valueRange.isEmpty());

        if (lockedThis.m_dataMinValue[component] == valueRange.min()
            && lockedThis.m_dataMaxValue[component] == valueRange.max())
        {
            continue;
        }

        minMaxChanged = true;

        lockedThis.m_minValue[component] = lockedThis.m_dataMinValue[component] = valueRange.min();
        lockedThis.m_maxValue[component] = lockedThis.m_dataMaxValue[component] = valueRange.max();
    }

    lockedThis.m_boundsValid = true;

    m_boundsUpdateMutex.unlock();

    if (minMaxChanged)
    {
        {
            // Don't emit minMaxChanged(), rely on dataMinMaxChanged only
            QSignalBlocker blocker(lockedThis);
            lockedThis.minMaxChangedEvent();
        }
        emit lockedThis.dataMinMaxChanged();
    }
}
