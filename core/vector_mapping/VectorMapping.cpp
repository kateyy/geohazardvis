#include "VectorMapping.h"

#include <cassert>

#include <core/DataSetHandler.h>
#include <core/vector_mapping/VectorMappingData.h>
#include <core/vector_mapping/VectorMappingRegistry.h>


VectorMapping::VectorMapping(RenderedData * renderedData)
    : m_renderedData(renderedData)
{
    m_vectors = VectorMappingRegistry::instance().createMappingsValidFor(m_renderedData);

    connect(&DataSetHandler::instance(), &DataSetHandler::rawVectorsChanged,
        this, &VectorMapping::updateAvailableVectors);
}

VectorMapping::~VectorMapping()
{
    qDeleteAll(m_vectors.values());
}

QList<QString> VectorMapping::vectorNames() const
{
    return m_vectors.keys();
}

const QMap<QString, VectorMappingData *> & VectorMapping::vectors() const
{
    return m_vectors;
}

const RenderedData * VectorMapping::renderedData() const
{
    return m_renderedData;
}

void VectorMapping::updateAvailableVectors()
{
    auto newlyCreated = VectorMappingRegistry::instance().createMappingsValidFor(m_renderedData);

    QMap<QString, VectorMappingData *> newValidList;

    for (VectorMappingData * vectors : newlyCreated)
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
