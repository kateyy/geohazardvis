#include "VectorsForSurfaceMappingRegistry.h"

#include <cassert>

#include "VectorsForSurfaceMapping.h"


VectorsForSurfaceMappingRegistry::VectorsForSurfaceMappingRegistry()
{
}

VectorsForSurfaceMappingRegistry::~VectorsForSurfaceMappingRegistry() = default;

VectorsForSurfaceMappingRegistry & VectorsForSurfaceMappingRegistry::instance()
{
    static VectorsForSurfaceMappingRegistry registry;
    return registry;
}

QMap<QString, VectorsForSurfaceMapping *> VectorsForSurfaceMappingRegistry::createMappingsValidFor(RenderedData * renderedData)
{
    QMap<QString, VectorsForSurfaceMapping *> validVectors;

    for (auto creator : m_mappingCreators)
    {
        QList<VectorsForSurfaceMapping *> vectors = creator(renderedData);

        for (VectorsForSurfaceMapping * v : vectors)
        {
            if (v->isValid())
                validVectors.insert(v->name(), v);
            else
                delete v;
        }
    }

    return validVectors;
}

const QMap<QString, VectorsForSurfaceMappingRegistry::MappingCreator> & VectorsForSurfaceMappingRegistry::mappingCreators()
{
    return m_mappingCreators;
}

bool VectorsForSurfaceMappingRegistry::registerImplementation(QString name, const MappingCreator & constructor)
{
    assert(!m_mappingCreators.contains(name));
    if (m_mappingCreators.contains(name))
        return false;

    m_mappingCreators.insert(name, constructor);
    return true;
}
