#include "ScalarsForColorMappingRegistry.h"

#include <cassert>

#include "ScalarsForColorMapping.h"


ScalarsForColorMappingRegistry::ScalarsForColorMappingRegistry()
{
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

    for (auto creator : m_mappingCreators)
    {
        QList<ScalarsForColorMapping *> scalars = creator(dataObjects);

        for (ScalarsForColorMapping * s : scalars)
        {
            if (s->isValid())
                validScalars.insert(s->name(), s);
            else
                delete s;
        }
    }

    return validScalars;
}

const QMap<QString, ScalarsForColorMappingRegistry::MappingCreator> & ScalarsForColorMappingRegistry::mappingCreators()
{
    return m_mappingCreators;
}

bool ScalarsForColorMappingRegistry::registerImplementation(QString name, const MappingCreator & creator)
{
    assert(!m_mappingCreators.contains(name));
    if (m_mappingCreators.contains(name))
        return false;

    m_mappingCreators.insert(name, creator);
    return true;
}
