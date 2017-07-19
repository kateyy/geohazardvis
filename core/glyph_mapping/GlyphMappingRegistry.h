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

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include <core/core_api.h>


class QString;
class RenderedData;
class GlyphMappingData;


class CORE_API GlyphMappingRegistry
{
public:
    static GlyphMappingRegistry & instance();

    using MappingCreator = std::function<std::vector<std::unique_ptr<GlyphMappingData>>(RenderedData & renderedData)>;
    bool registerImplementation(const QString & name, const MappingCreator & creator);

    /** retrieve a list of vectors that are applicable for the rendered data object */
    std::map<QString, std::unique_ptr<GlyphMappingData>> createMappingsValidFor(RenderedData & renderedData) const;

private:
    GlyphMappingRegistry();
    ~GlyphMappingRegistry();

    std::map<QString, MappingCreator> m_mappingCreators;
};
