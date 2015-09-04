#include "GlyphMapping.h"

#include <cassert>

#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/glyph_mapping/GlyphMappingData.h>
#include <core/glyph_mapping/GlyphMappingRegistry.h>


GlyphMapping::GlyphMapping(RenderedData & renderedData)
    : m_renderedData(renderedData)
{
    m_vectors = GlyphMappingRegistry::instance().createMappingsValidFor(m_renderedData);

    // after adding an attribute array, updateAvailableVectors() will again trigger the Modified event on point/cell/field data
    // in that case, the next event should only be processed after updateAvailableVectors() finished
    connect(&renderedData.dataObject(), &DataObject::attributeArraysChanged, this, &GlyphMapping::updateAvailableVectors, Qt::QueuedConnection);
}

GlyphMapping::~GlyphMapping() = default;

QList<QString> GlyphMapping::vectorNames() const
{
    QList<QString> result;
    for (auto & v : m_vectors)
        result << v.first;

    return result;
}

QList<GlyphMappingData *> GlyphMapping::vectors() const
{
    QList<GlyphMappingData *> result;
    for (auto & v : m_vectors)
        result << v.second.get();

    return result;
}

const RenderedData & GlyphMapping::renderedData() const
{
    return m_renderedData;
}

void GlyphMapping::updateAvailableVectors()
{
    decltype(m_vectors) newlyCreated = GlyphMappingRegistry::instance().createMappingsValidFor(m_renderedData);
    decltype(m_vectors) newValidList;

    bool _vectorsChanged = false;

    auto oldInstances = std::move(m_vectors);

    for (auto & newIt : newlyCreated)
    {
        auto & vectorsName = newIt.first;
        auto & newVectors = newIt.second;

        auto oldIt = oldInstances.find(vectorsName);

        if (oldIt != oldInstances.end())
        {
            // not new -> keep old
            newVectors.reset();
            newValidList.emplace(vectorsName, std::move(oldIt->second));
            oldInstances.erase(oldIt);
        }
        else
        {
            // it's really new, so use it
            newValidList.emplace(vectorsName, std::move(newVectors));
            _vectorsChanged = true;
        }
    }

    _vectorsChanged = _vectorsChanged || !oldInstances.empty();   // something new or obsolete old instances?

    oldInstances.clear();
    m_vectors = std::move(newValidList);

    if (_vectorsChanged)
        emit vectorsChanged();
}
