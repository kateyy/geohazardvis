#include "CoordinateValueMapping.h"

#include "core/Input.h"
#include "core/data_objects/PolyDataObject.h"

#include "ScalarsForColorMappingRegistry.h"

const QString CoordinateXValueMapping::s_name = "x values";
const bool CoordinateXValueMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<CoordinateXValueMapping>);
const QString CoordinateYValueMapping::s_name = "y values";
const bool CoordinateYValueMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<CoordinateYValueMapping>);
const QString CoordinateZValueMapping::s_name = "z values";
const bool CoordinateZValueMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<CoordinateZValueMapping>);


AbstractCoordinateValueMapping::AbstractCoordinateValueMapping(const QList<DataObject *> & dataObjects)
    : ScalarsForColorMapping(dataObjects)
{
    for (DataObject * dataObject : dataObjects)
    {
        PolyDataObject * polyDataObject = dynamic_cast<PolyDataObject*>(dataObject);

        if (!polyDataObject)
            break;

        m_dataObjects << polyDataObject;
    }

    // applicable for list of PolyDataObjects only
    if (!m_dataObjects.length() == dataObjects.length())
        m_dataObjects.clear();
}

AbstractCoordinateValueMapping::~AbstractCoordinateValueMapping() = default;

bool AbstractCoordinateValueMapping::usesGradients() const
{
    return true;
}

bool AbstractCoordinateValueMapping::isValid() const
{
    return !m_dataObjects.isEmpty();
}

CoordinateXValueMapping::CoordinateXValueMapping(const QList<DataObject *> & dataObjects)
    : AbstractCoordinateValueMapping(dataObjects)
{
}

QString CoordinateXValueMapping::name() const
{
    return s_name;
}

void CoordinateXValueMapping::updateBounds()
{
    for (PolyDataObject * dataObject : m_dataObjects)
    {
        m_minValue = std::min(m_minValue, dataObject->input()->bounds()[0]);
        m_maxValue = std::max(m_maxValue, dataObject->input()->bounds()[1]);
    }
}


CoordinateYValueMapping::CoordinateYValueMapping(const QList<DataObject *> & dataObjects)
    : AbstractCoordinateValueMapping(dataObjects)
{
}

QString CoordinateYValueMapping::name() const
{
    return s_name;
}

void CoordinateYValueMapping::updateBounds()
{
    for (PolyDataObject * dataObject : m_dataObjects)
    {
        m_minValue = std::min(m_minValue, dataObject->input()->bounds()[2]);
        m_maxValue = std::max(m_maxValue, dataObject->input()->bounds()[3]);
    }
}


CoordinateZValueMapping::CoordinateZValueMapping(const QList<DataObject *> & dataObjects)
    : AbstractCoordinateValueMapping(dataObjects)
{
}

QString CoordinateZValueMapping::name() const
{
    return s_name;
}

void CoordinateZValueMapping::updateBounds()
{
    for (PolyDataObject * dataObject : m_dataObjects)
    {
        m_minValue = std::min(m_minValue, dataObject->input()->bounds()[4]);
        m_maxValue = std::max(m_maxValue, dataObject->input()->bounds()[5]);
    }
}