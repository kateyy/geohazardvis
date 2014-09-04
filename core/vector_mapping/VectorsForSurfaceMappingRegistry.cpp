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

    for (auto constr : m_mappingConstructors)
    {
        VectorsForSurfaceMapping * vectors = constr(renderedData);

        if (vectors->isValid())
            validVectors.insert(vectors->name(), vectors);
        else
            delete vectors;
    }

    return validVectors;
}

const QMap<QString, VectorsForSurfaceMappingRegistry::MappingConstructor> & VectorsForSurfaceMappingRegistry::mappingConstructors()
{
    return m_mappingConstructors;
}

bool VectorsForSurfaceMappingRegistry::registerImplementation(QString name, const MappingConstructor & constructor)
{
    assert(!m_mappingConstructors.contains(name));
    if (m_mappingConstructors.contains(name))
        return false;

    m_mappingConstructors.insert(name, constructor);
    return true;
}
