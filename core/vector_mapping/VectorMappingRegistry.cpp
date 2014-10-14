#include "VectorMappingRegistry.h"

#include <cassert>

#include "VectorMappingData.h"


VectorMappingRegistry::VectorMappingRegistry()
{
}

VectorMappingRegistry::~VectorMappingRegistry() = default;

VectorMappingRegistry & VectorMappingRegistry::instance()
{
    static VectorMappingRegistry registry;
    return registry;
}

QMap<QString, VectorMappingData *> VectorMappingRegistry::createMappingsValidFor(RenderedData * renderedData)
{
    QMap<QString, VectorMappingData *> validVectors;

    for (auto creator : m_mappingCreators)
    {
        QList<VectorMappingData *> vectors = creator(renderedData);

        for (VectorMappingData * v : vectors)
        {
            if (v->isValid())
                validVectors.insert(v->name(), v);
            else
                delete v;
        }
    }

    return validVectors;
}

const QMap<QString, VectorMappingRegistry::MappingCreator> & VectorMappingRegistry::mappingCreators()
{
    return m_mappingCreators;
}

bool VectorMappingRegistry::registerImplementation(QString name, const MappingCreator & constructor)
{
    assert(!m_mappingCreators.contains(name));
    if (m_mappingCreators.contains(name))
        return false;

    m_mappingCreators.insert(name, constructor);
    return true;
}
