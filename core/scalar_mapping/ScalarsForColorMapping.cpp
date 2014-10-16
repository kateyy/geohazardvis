#include "ScalarsForColorMapping.h"

#include <cassert>
#include <limits>

#include <QSet>

#include <core/data_objects/DataObject.h>


ScalarsForColorMapping::ScalarsForColorMapping(const QList<DataObject *> & dataObjects, vtkIdType numDataComponents)
    : m_dataObjects(dataObjects)
    , m_startingIndex(0)
    , m_dataComponent(0)
    , m_numDataComponents(numDataComponents)
    , m_dataMinValue(numDataComponents, std::numeric_limits<double>::max())
    , m_dataMaxValue(numDataComponents, std::numeric_limits<double>::lowest())
    , m_minValue(numDataComponents, std::numeric_limits<double>::max())
    , m_maxValue(numDataComponents, std::numeric_limits<double>::lowest())
{
}

ScalarsForColorMapping::~ScalarsForColorMapping() = default;

QString ScalarsForColorMapping::scalarsName() const
{
    return "";
}

vtkIdType ScalarsForColorMapping::numDataComponents() const
{
    return m_numDataComponents;
}

vtkIdType ScalarsForColorMapping::dataComponent() const
{
    return m_dataComponent;
}

void ScalarsForColorMapping::setDataComponent(vtkIdType component)
{
    assert(0 <= component && component < m_numDataComponents);
    if (m_dataComponent == component)
        return;

    m_dataComponent = component;

    dataComponentChangedEvent();
}

vtkIdType ScalarsForColorMapping::maximumStartingIndex()
{
    return 0;
}

vtkIdType ScalarsForColorMapping::startingIndex() const
{
    assert(m_startingIndex <= const_cast<ScalarsForColorMapping *>(this)->maximumStartingIndex());
    return m_startingIndex;
}

void ScalarsForColorMapping::setStartingIndex(vtkIdType index)
{
    vtkIdType newIndex = std::max(0ll, std::min(index, maximumStartingIndex()));

    if (newIndex == m_startingIndex)
        return;

    m_startingIndex = newIndex;

    startingIndexChangedEvent();
}

QList<DataObject *> ScalarsForColorMapping::dataObjects() const
{
    return m_dataObjects;
}

void ScalarsForColorMapping::rearrangeDataObjets(QList<DataObject *> dataObjects)
{
    assert(dataObjects.toSet() == m_dataObjects.toSet());

    if (m_dataObjects == dataObjects)
        return;

    m_dataObjects = dataObjects;

    objectOrderChangedEvent();
}

void ScalarsForColorMapping::initialize()
{
    for (DataObject * dataObject : m_dataObjects)
        connect(dataObject, &DataObject::valueRangeChanged, this, &ScalarsForColorMapping::updateBounds);

    updateBounds();
}

vtkAlgorithm * ScalarsForColorMapping::createFilter(DataObject * /*dataObject*/)
{
    return nullptr;
}

bool ScalarsForColorMapping::usesFilter() const
{
    return false;
}

void ScalarsForColorMapping::configureDataObjectAndMapper(DataObject * /*dataObject*/, vtkMapper * /*mapper*/)
{
}

double ScalarsForColorMapping::dataMinValue(vtkIdType component) const
{
    assert(m_dataMinValue <= m_dataMaxValue && component < m_numDataComponents);
    if (component < 0)
        component = m_dataComponent;
    return m_dataMinValue[component];
}

double ScalarsForColorMapping::dataMaxValue(vtkIdType component) const
{
    assert(m_dataMinValue <= m_dataMaxValue && component < m_numDataComponents);
    if (component < 0)
        component = m_dataComponent;
    return m_dataMaxValue[component];
}

double ScalarsForColorMapping::minValue(vtkIdType component) const
{
    if (component < 0)
        component = m_dataComponent;
    return m_minValue[component];
}

void ScalarsForColorMapping::setMinValue(double value, vtkIdType component)
{
    if (component < 0)
        component = m_dataComponent;

    m_minValue[component] = std::min(std::max(m_dataMinValue[component], value), m_dataMaxValue[component]);

    minMaxChangedEvent();
}

double ScalarsForColorMapping::maxValue(vtkIdType component) const
{
    if (component < 0)
        component = m_dataComponent;

    return m_maxValue[component];
}

void ScalarsForColorMapping::setMaxValue(double value, vtkIdType component)
{
    if (component < 0)
        component = m_dataComponent;

    m_maxValue[component] = std::min(std::max(m_dataMinValue[component], value), m_dataMaxValue[component]);

    minMaxChangedEvent();
}

void ScalarsForColorMapping::setDataMinMaxValue(const double minMax[2], vtkIdType component)
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

void ScalarsForColorMapping::setDataMinMaxValue(double min, double max, vtkIdType component)
{
    double minMax[2] = { min, max };
    setDataMinMaxValue(minMax, component);
}

void ScalarsForColorMapping::dataComponentChangedEvent()
{
}

void ScalarsForColorMapping::minMaxChangedEvent()
{
    emit minMaxChanged();
}

void ScalarsForColorMapping::startingIndexChangedEvent()
{
}

void ScalarsForColorMapping::objectOrderChangedEvent()
{
}
