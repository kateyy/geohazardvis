#include "AbstractVisualizedData.h"

#include <cassert>

#include <core/data_objects/DataObject.h>


AbstractVisualizedData::AbstractVisualizedData(DataObject * dataObject)
    : QObject()
    , m_dataObject(dataObject)
    , m_isVisible(true)
{
    assert(m_dataObject);

    connect(dataObject, &DataObject::dataChanged, this, &AbstractVisualizedData::geometryChanged);
}

bool AbstractVisualizedData::isVisible() const
{
    return m_isVisible;
}

void AbstractVisualizedData::setVisible(bool visible)
{
    m_isVisible = visible;

    visibilityChangedEvent(visible);

    emit visibilityChanged(visible);
}

void AbstractVisualizedData::visibilityChangedEvent(bool /*visible*/)
{
}

DataObject * AbstractVisualizedData::dataObject()
{
    return m_dataObject;
}

const DataObject * AbstractVisualizedData::dataObject() const
{
    return m_dataObject;
}
