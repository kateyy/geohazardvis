#include "ScalarsForColorMappingRegistry.h"

#include <cassert>

#include "ScalarsForColorMapping.h"

#include "DefaultColorMapping.h"
#include "CoordinateValueMapping.h"


ScalarsForColorMappingRegistry::ScalarsForColorMappingRegistry()
{
    // HACK : work around discarded(?) object files, registrations not called
    DefaultColorMapping({});
    CoordinateXValueMapping({});
}

ScalarsForColorMappingRegistry::~ScalarsForColorMappingRegistry() = default;

ScalarsForColorMappingRegistry & ScalarsForColorMappingRegistry::instance()
{
    static ScalarsForColorMappingRegistry registry;
    return registry;
}

QMap<QString, ScalarsForColorMapping *> ScalarsForColorMappingRegistry::createMappingsValidFor(const QList<DataObject*> & dataObjects)
{
    QMap<QString, ScalarsForColorMapping *> validScalars;

    for (auto constr : m_mappingConstructors)
    {
        ScalarsForColorMapping * scalars = constr(dataObjects);

        if (scalars->isValid())
            validScalars.insert(scalars->name(), scalars);
        else
            delete scalars;
    }

    return validScalars;
}

const QMap<QString, ScalarsForColorMappingRegistry::MappingConstructor> & ScalarsForColorMappingRegistry::mappingConstructors()
{
    return m_mappingConstructors;
}

bool ScalarsForColorMappingRegistry::registerImplementation(QString name, const MappingConstructor & constructor)
{
    assert(!m_mappingConstructors.contains(name));
    if (m_mappingConstructors.contains(name))
        return false;

    m_mappingConstructors.insert(name, constructor);
    return true;
}
