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

class AbstractVisualizedData;
class ColorMappingData;


class CORE_API ColorMappingRegistry
{
public:
    static ColorMappingRegistry & instance();

    using MappingCreator = std::function<std::vector<std::unique_ptr<ColorMappingData>>(
        const std::vector<AbstractVisualizedData *> & visualizedData)>;
    bool registerImplementation(const QString & name, const MappingCreator & creator);

    /** retrieve a list of scalars extractions that are applicable for the specified data object list */
    std::map<QString, std::unique_ptr<ColorMappingData>> createMappingsValidFor(
        const std::vector<AbstractVisualizedData*> & visualizedData) const;

private:
    ColorMappingRegistry();
    ~ColorMappingRegistry();

    std::map<QString, MappingCreator> m_mappingCreators;

private:
    ColorMappingRegistry(const ColorMappingRegistry &) = delete;
    void operator=(const ColorMappingRegistry &) = delete;
};
