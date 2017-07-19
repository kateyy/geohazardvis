/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GlyphMapping.h"

#include <cassert>

#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/glyph_mapping/GlyphMappingData.h>
#include <core/glyph_mapping/GlyphMappingRegistry.h>


GlyphMapping::GlyphMapping(RenderedData & renderedData)
    : m_renderedData{ renderedData }
{
    m_vectors = GlyphMappingRegistry::instance().createMappingsValidFor(m_renderedData);

    connect(&renderedData.dataObject(), &DataObject::attributeArraysChanged, this, &GlyphMapping::updateAvailableVectors);
}

GlyphMapping::~GlyphMapping() = default;

QStringList GlyphMapping::vectorNames() const
{
    QStringList result;
    for (const auto & v : m_vectors)
    {
        result << v.first;
    }

    return result;
}

std::vector<GlyphMappingData *> GlyphMapping::vectors() const
{
    std::vector<GlyphMappingData *> result;
    for (const auto & v : m_vectors)
    {
        result.emplace_back(v.second.get());
    }

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
    {
        emit vectorsChanged();
    }
}
