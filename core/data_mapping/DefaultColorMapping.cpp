#include "DefaultColorMapping.h"


namespace
{
QString mappingName = "default color";

bool isRegistered = ScalarsForColorMapping::registerImplementation(
                        mappingName,
                        ScalarsForColorMapping::newInstance<DefaultColorMapping>);
}

DefaultColorMapping::DefaultColorMapping(const QList<DataObject *> & dataObjects)
    : ScalarsForColorMapping(dataObjects)
{
}

DefaultColorMapping::~DefaultColorMapping() = default;

QString DefaultColorMapping::name() const
{
    return mappingName;
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
