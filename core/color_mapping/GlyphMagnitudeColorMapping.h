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

#include <map>

#include <core/color_mapping/GlyphColorMapping.h>


class vtkVectorNorm;


class CORE_API GlyphMagnitudeColorMapping : public GlyphColorMapping
{
public:
    GlyphMagnitudeColorMapping(
        const std::vector<AbstractVisualizedData *> & visualizedData,
        const QString & vectorName,
        const std::map<RenderedData3D *, GlyphMappingData *> & glyphMappingData);
    ~GlyphMagnitudeColorMapping() override;

    QString name() const override;
    QString scalarsName(AbstractVisualizedData & vis) const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData & visualizedData, unsigned int port = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper, unsigned int port = 0) override;

protected:
    static std::vector<std::unique_ptr<ColorMappingData>> newInstances(const std::vector<AbstractVisualizedData*> & visualizedData);

    std::vector<ValueRange<>> updateBounds() override;

private:
    static const bool s_isRegistered;

    const QString m_vectorName;
    std::map<AbstractVisualizedData *, vtkSmartPointer<vtkAlgorithm>> m_filters;

private:
    Q_DISABLE_COPY(GlyphMagnitudeColorMapping)
};
