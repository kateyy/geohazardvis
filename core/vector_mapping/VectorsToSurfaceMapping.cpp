#include "VectorsToSurfaceMapping.h"

#include <cassert>

#include "VectorsForSurfaceMappingRegistry.h"


VectorsToSurfaceMapping::VectorsToSurfaceMapping(RenderedData * renderedData)
    : m_renderedData(renderedData)
{
    m_vectors = VectorsForSurfaceMappingRegistry::instance().createMappingsValidFor(renderedData);
}

QList<QString> VectorsToSurfaceMapping::vectorNames() const
{
    return m_vectors.keys();
}

const QMap<QString, VectorsForSurfaceMapping *> & VectorsToSurfaceMapping::vectors() const
{
    return m_vectors;
}

const RenderedData * VectorsToSurfaceMapping::renderedData() const
{
    return m_renderedData;
}
