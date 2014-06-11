#include "DefaultColorMapping.h"

#include "ScalarsForColorMappingRegistry.h"


const QString DefaultColorMapping::s_name = "default color";

const bool DefaultColorMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<DefaultColorMapping>);

DefaultColorMapping::DefaultColorMapping(const QList<DataObject *> & dataObjects)
    : ScalarsForColorMapping(dataObjects)
{
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
    m_minValue = 0;
    m_maxValue = 0;
}

bool DefaultColorMapping::isValid() const
{
    return true;
}
