#include "ScalarsForColorMapping.h"

#include <cassert>
#include <limits>


ScalarsForColorMapping::ScalarsForColorMapping(const QList<DataObject *> & /*dataObjects*/)
    : m_dataMinValue(std::numeric_limits<double>::max())
    , m_dataMaxValue(std::numeric_limits<double>::lowest())
{
}

void ScalarsForColorMapping::initialize()
{
    updateBounds();
}

ScalarsForColorMapping::~ScalarsForColorMapping() = default;

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
}

double ScalarsForColorMapping::maxValue() const
{
    return m_maxValue;
}

void ScalarsForColorMapping::setMaxValue(double value)
{
    m_maxValue = std::min(std::max(m_dataMinValue, value), m_dataMaxValue);
}

void ScalarsForColorMapping::updateBounds()
{
    m_minValue = m_dataMinValue;
    m_maxValue = m_dataMaxValue;
}
