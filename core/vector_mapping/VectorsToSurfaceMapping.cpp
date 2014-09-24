#include "VectorsToSurfaceMapping.h"

#include <cassert>

#include <core/DataSetHandler.h>
#include <core/vector_mapping/VectorsForSurfaceMapping.h>
#include <core/vector_mapping/VectorsForSurfaceMappingRegistry.h>


VectorsToSurfaceMapping::VectorsToSurfaceMapping(RenderedData * renderedData)
    : m_renderedData(renderedData)
{
    m_vectors = VectorsForSurfaceMappingRegistry::instance().createMappingsValidFor(m_renderedData);

    connect(&DataSetHandler::instance(), &DataSetHandler::attributeVectorsChanged,
        this, &VectorsToSurfaceMapping::updateAvailableVectors);
}

VectorsToSurfaceMapping::~VectorsToSurfaceMapping()
{
    qDeleteAll(m_vectors.values());
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

void VectorsToSurfaceMapping::updateAvailableVectors()
{
    auto newlyCreated = VectorsForSurfaceMappingRegistry::instance().createMappingsValidFor(m_renderedData);

    QMap<QString, VectorsForSurfaceMapping *> newValidList;

    for (VectorsForSurfaceMapping * vectors : newlyCreated)
    {
        auto * oldInstance = m_vectors.value(vectors->name(), nullptr);

        // not new -> keep old
        if (oldInstance)
        {
            delete vectors;
            // move to the new list, so later, we can delete all vectors that are not valid anymore
            newValidList.insert(oldInstance->name(), oldInstance);
            m_vectors.remove(oldInstance->name());
        }
        else
        {
            // it's really new, so use it
            newValidList.insert(vectors->name(), vectors);
        }
    }

    qDeleteAll(m_vectors.values());
    m_vectors = newValidList;

    emit vectorsChanged();
}
