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

#include "GlyphColorMapping.h"

#include <cassert>

#include <vtkLookupTable.h>

#include <core/types.h>
#include <core/glyph_mapping/GlyphMappingData.h>
#include <core/rendered_data/RenderedData3D.h>


GlyphColorMapping::GlyphColorMapping(
    const std::vector<AbstractVisualizedData *> & visualizedData,
    const std::map<RenderedData3D *, GlyphMappingData *> & glyphMappingData,
    int numDataComponents)
    : ColorMappingData(visualizedData, numDataComponents)
    , m_glyphMappingData{ glyphMappingData }
{
}

GlyphColorMapping::~GlyphColorMapping() = default;

IndexType GlyphColorMapping::scalarsAssociation(AbstractVisualizedData & vis) const
{
    auto it = m_glyphMappingData.find(static_cast<RenderedData3D *>(&vis));
    if (it != m_glyphMappingData.end() && it->second)
    {
        assert(it->second);
        return it->second->scalarsAssociation();
    }

    return IndexType::invalid;
}

void GlyphColorMapping::assignToVisualization()
{
    for (const auto & pair : m_glyphMappingData)
    {
        assert(pair.second);
        pair.second->setColorMappingData(this);
        pair.second->setColorMappingGradient(m_lut);
    }
}

void GlyphColorMapping::unassignFromVisualization()
{
    for (const auto & pair : m_glyphMappingData)
    {
        assert(pair.second);
        pair.second->setColorMappingData(nullptr);
        pair.second->setColorMappingGradient(nullptr);
    }
}
