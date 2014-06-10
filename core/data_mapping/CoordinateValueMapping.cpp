#include "CoordinateValueMapping.h"

#include "core/Input.h"
#include "core/data_objects/PolyDataObject.h"


namespace
{
QString xMappingName = "x values";

bool isRegistered = ScalarsForColorMapping::registerImplementation(
    xMappingName,
    ScalarsForColorMapping::newInstance<CoordinateXValueMapping>);
}


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
    return xMappingName;
}

void CoordinateXValueMapping::updateBounds()
{
    for (PolyDataObject * dataObject : m_dataObjects)
    {
        m_minValue = std::min(m_minValue, dataObject->input()->bounds()[0]);
        m_maxValue = std::max(m_maxValue, dataObject->input()->bounds()[1]);
    }
}
