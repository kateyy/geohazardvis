#include "DefaultColorMapping.h"

#include "ScalarsForColorMappingRegistry.h"

#include <core/data_objects/PolyDataObject.h>


const QString DefaultColorMapping::s_name = "user-defined color";

const bool DefaultColorMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<DefaultColorMapping>);

DefaultColorMapping::DefaultColorMapping(const QList<DataObject *> & dataObjects)
    : ScalarsForColorMapping(dataObjects)
{
    int validObjects = 0;

    for (DataObject * dataObject : dataObjects)
    {
        PolyDataObject * polyDataObject = dynamic_cast<PolyDataObject*>(dataObject);

        if (!polyDataObject)
            break;

        validObjects++;
    }

    // applicable for list of PolyDataObjects only
    m_valid = validObjects == dataObjects.length();
}

DefaultColorMapping::~DefaultColorMapping() = default;

QString DefaultColorMapping::name() const
{
    return s_name;
}

bool DefaultColorMapping::usesGradients() const
{
    return false;
}

void DefaultColorMapping::updateBounds()
{
    m_dataMinValue = 0;
    m_dataMaxValue = 0;
    ScalarsForColorMapping::updateBounds();
}

bool DefaultColorMapping::isValid() const
{
    return m_valid;
}
