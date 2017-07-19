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

#include "GlyphColorMappingGlyphListener.h"

#include <core/rendered_data/RenderedData3D.h>
#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>
#include <core/utility/qthelper.h>


GlyphColorMappingGlyphListener::GlyphColorMappingGlyphListener(QObject * parent)
    : QObject(parent)
{
}

GlyphColorMappingGlyphListener::~GlyphColorMappingGlyphListener() = default;

void GlyphColorMappingGlyphListener::setData(const std::vector<AbstractVisualizedData *> & visualizedData)
{
    m_data.clear();

    disconnectAll(m_connects);

    for (auto vis : visualizedData)
    {
        auto rendered3D = dynamic_cast<RenderedData3D *>(vis);
        if (!rendered3D)
        {
            continue;
        }

        m_data.emplace_back(rendered3D);

        auto & glyphMapping = rendered3D->glyphMapping();
        m_connects.emplace_back(connect(&glyphMapping, &GlyphMapping::vectorsChanged,
            this, &GlyphColorMappingGlyphListener::glyphMappingChanged));

        for (auto data : glyphMapping.vectors())
        {
            m_connects.emplace_back(connect(data, &GlyphMappingData::visibilityChanged,
                this, &GlyphColorMappingGlyphListener::glyphMappingChanged));
        }
    }
}
