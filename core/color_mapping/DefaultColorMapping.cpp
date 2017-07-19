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

#include <core/color_mapping/DefaultColorMapping.h>

#include <QString>

#include <vtkMapper.h>

#include <core/types.h>
#include <core/utility/DataExtent.h>


const QString DefaultColorMapping::s_name = "user-defined color";

DefaultColorMapping::DefaultColorMapping(const std::vector<AbstractVisualizedData *> & visualizedData)
    : ColorMappingData(visualizedData)
{
    m_isValid = true;
}

DefaultColorMapping::~DefaultColorMapping() = default;

QString DefaultColorMapping::name() const
{
    return s_name;
}

void DefaultColorMapping::configureMapper(
    AbstractVisualizedData & visualizedData,
    vtkAbstractMapper & mapper,
    unsigned int port)
{
    ColorMappingData::configureMapper(visualizedData, mapper, port);

    if (auto m = vtkMapper::SafeDownCast(&mapper))
    {
        m->ScalarVisibilityOff();
    }
}

std::vector<ValueRange<>> DefaultColorMapping::updateBounds()
{
    return{ ValueRange<>({ 0.0, 0.0 }) };
}
