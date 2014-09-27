#include "ScalarsForColorMapping.h"

#include <cassert>
#include <limits>

#include <QSet>

#include <core/data_objects/DataObject.h>


ScalarsForColorMapping::ScalarsForColorMapping(const QList<DataObject *> & dataObjects)
    : m_dataObjects(dataObjects)
    , m_startingIndex(0)
    , m_dataMinValue(std::numeric_limits<double>::max())
    , m_dataMaxValue(std::numeric_limits<double>::lowest())
    , m_minValue(std::numeric_limits<double>::max())
    , m_maxValue(std::numeric_limits<double>::lowest())
{
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

ScalarsForColorMapping::~ScalarsForColorMapping() = default;

vtkAlgorithm * ScalarsForColorMapping::createFilter(DataObject * /*dataObject*/)
{
    return nullptr;
}

bool ScalarsForColorMapping::usesFilter() const
{
    return false;
}

void ScalarsForColorMapping::configureDataObjectAndMapper(DataObject * dataObject, vtkMapper * /*mapper*/)
{
    assert(m_dataObjects.contains(dataObject));
}

double ScalarsForColorMapping::dataMinValue() const
{
    assert(m_dataMinValue <= m_dataMaxValue);
    return m_dataMinValue;
}

double ScalarsForColorMapping::dataMaxValue() const
{
    assert(m_dataMinValue <= m_dataMaxValue);
    return m_dataMaxValue;
}

double ScalarsForColorMapping::minValue() const
{
    return m_minValue;
}

void ScalarsForColorMapping::setMinValue(double value)
{
    m_minValue = std::min(std::max(m_dataMinValue, value), m_dataMaxValue);

    minMaxChangedEvent();
}

double ScalarsForColorMapping::maxValue() const
{
    return m_maxValue;
}

void ScalarsForColorMapping::setMaxValue(double value)
{
    m_maxValue = std::min(std::max(m_dataMinValue, value), m_dataMaxValue);

    minMaxChangedEvent();
}

void ScalarsForColorMapping::updateBounds()
{
    bool minMaxChanged = false;
    
    // reset user selected ranges only if really needed
    if (m_minValue < m_dataMinValue || m_minValue > m_dataMaxValue)
    {
        m_minValue = m_dataMinValue;
        minMaxChanged = true;
    }
    if (m_maxValue < m_dataMinValue || m_maxValue > m_dataMaxValue)
    {
        m_maxValue = m_dataMaxValue;
        minMaxChanged = true;
    }
    
    if (minMaxChanged)
        minMaxChangedEvent();

    emit dataMinMaxChanged();
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
