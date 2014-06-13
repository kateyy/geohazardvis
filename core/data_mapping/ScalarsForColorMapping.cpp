#include "ScalarsForColorMapping.h"

#include <cassert>
#include <limits>


ScalarsForColorMapping::ScalarsForColorMapping(const QList<DataObject *> & /*dataObjects*/)
    : m_minValue(std::numeric_limits<double>::max())
    , m_maxValue(std::numeric_limits<double>::lowest())
{
}

void ScalarsForColorMapping::initialize()
{
    updateBounds();
}

ScalarsForColorMapping::~ScalarsForColorMapping() = default;

double ScalarsForColorMapping::minValue() const
{
    assert(m_minValue <= m_maxValue);
    return m_minValue;
}

double ScalarsForColorMapping::maxValue() const
{
    assert(m_minValue <= m_maxValue);
    return m_maxValue;
}

void ScalarsForColorMapping::updateBounds()
{
}
