#include "GlyphMapping.h"

#include <cassert>

#include <core/DataSetHandler.h>
#include <core/glyph_mapping/GlyphMappingData.h>
#include <core/glyph_mapping/GlyphMappingRegistry.h>


GlyphMapping::GlyphMapping(RenderedData * renderedData)
    : m_renderedData(renderedData)
{
    m_vectors = GlyphMappingRegistry::instance().createMappingsValidFor(m_renderedData);

    connect(&DataSetHandler::instance(), &DataSetHandler::rawVectorsChanged,
        this, &GlyphMapping::updateAvailableVectors);
}

GlyphMapping::~GlyphMapping()
{
    qDeleteAll(m_vectors.values());
}

QList<QString> GlyphMapping::vectorNames() const
{
    return m_vectors.keys();
}

const QMap<QString, GlyphMappingData *> & GlyphMapping::vectors() const
{
    return m_vectors;
}

const RenderedData * GlyphMapping::renderedData() const
{
    return m_renderedData;
}

void GlyphMapping::updateAvailableVectors()
{
    auto newlyCreated = GlyphMappingRegistry::instance().createMappingsValidFor(m_renderedData);

    QMap<QString, GlyphMappingData *> newValidList;

    for (GlyphMappingData * vectors : newlyCreated)
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
