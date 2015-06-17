#include "GlyphMapping.h"

#include <cassert>

#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/glyph_mapping/GlyphMappingData.h>
#include <core/glyph_mapping/GlyphMappingRegistry.h>


GlyphMapping::GlyphMapping(RenderedData * renderedData)
    : m_renderedData(renderedData)
{
    m_vectors = GlyphMappingRegistry::instance().createMappingsValidFor(m_renderedData);

    // after adding an attribute array, updateAvailableVectors() will again trigger the Modified event on point/cell/field data
    // in that case, the next event should only be processed after updateAvailableVectors() finished
    connect(renderedData->dataObject(), &DataObject::attributeArraysChanged, this, &GlyphMapping::updateAvailableVectors, Qt::QueuedConnection);
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

    bool _vectorsChanged = false;

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
            _vectorsChanged = true;
        }
    }

    _vectorsChanged = _vectorsChanged || !m_vectors.isEmpty();   // something new or old vectors to delete?

    qDeleteAll(m_vectors.values());
    m_vectors = newValidList;

    if (_vectorsChanged)
        emit vectorsChanged();
}
