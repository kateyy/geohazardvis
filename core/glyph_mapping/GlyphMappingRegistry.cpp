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

#include "GlyphMappingRegistry.h"

#include <cassert>

#include "GlyphMappingData.h"


GlyphMappingRegistry::GlyphMappingRegistry()
{
}

GlyphMappingRegistry::~GlyphMappingRegistry() = default;

GlyphMappingRegistry & GlyphMappingRegistry::instance()
{
    static GlyphMappingRegistry registry;
    return registry;
}

std::map<QString, std::unique_ptr<GlyphMappingData>> GlyphMappingRegistry::createMappingsValidFor(RenderedData & renderedData) const
{
    std::map<QString, std::unique_ptr<GlyphMappingData>> validVectors;

    for (auto && creator : m_mappingCreators)
    {
        auto vectors = creator.second(renderedData);

        for (auto & v : vectors)
        {
            if (v->isValid())
            {
                validVectors.emplace(v->name(), std::move(v));
            }
        }
    }

    return validVectors;
}

bool GlyphMappingRegistry::registerImplementation(const QString & name, const MappingCreator & creator)
{
    auto it = m_mappingCreators.find(name);
    assert(it == m_mappingCreators.end());
    if (it != m_mappingCreators.end())
    {
        return false;
    }

    m_mappingCreators.emplace(name, creator);
    return true;
}
