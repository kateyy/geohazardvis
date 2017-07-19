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

#include "ColorMappingRegistry.h"

#include <cassert>

#include <core/color_mapping/ColorMappingData.h>


ColorMappingRegistry::ColorMappingRegistry()
{
}

ColorMappingRegistry::~ColorMappingRegistry() = default;

ColorMappingRegistry & ColorMappingRegistry::instance()
{
    static ColorMappingRegistry registry;
    return registry;
}

std::map<QString, std::unique_ptr<ColorMappingData>> ColorMappingRegistry::createMappingsValidFor(
    const std::vector<AbstractVisualizedData *> & visualizedData) const
{
    std::map<QString, std::unique_ptr<ColorMappingData>> validScalars;

    for (const auto & creator : m_mappingCreators)
    {
        auto scalars = creator.second(visualizedData);

        for (auto & s : scalars)
        {
            if (s->isValid())
            {
                validScalars.emplace(s->name(), std::move(s));
            }
        }
    }

    return validScalars;
}

bool ColorMappingRegistry::registerImplementation(const QString & name, const MappingCreator & creator)
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
