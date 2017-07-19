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

#include <core/color_mapping/ColorMappingData.h>


/** "Null"-Color Mapping that is used when color mapping is disabled.

This ColorMappingData is not directly selectable by the user, thus is it not registered in the ColorMappingRegistry.
Instead, it is activated when disabling color mapping and will allows the user to configure surface colors directly.
*/
class CORE_API DefaultColorMapping : public ColorMappingData
{
public:
    explicit DefaultColorMapping(const std::vector<AbstractVisualizedData *> & visualizedData);
    ~DefaultColorMapping() override;

    QString name() const override;

    void configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper, unsigned int port = 0) override;

protected:
    std::vector<ValueRange<>> updateBounds() override;

private:
    static const QString s_name;

private:
    Q_DISABLE_COPY(DefaultColorMapping)
};
