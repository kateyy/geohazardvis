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

#include <core/glyph_mapping/GlyphMappingData.h>


class vtkAssignAttribute;
class vtkDataArray;
class RenderedVectorGrid3D;


class CORE_API Grid3dGlyphMapping : public GlyphMappingData
{
public:
    Grid3dGlyphMapping(RenderedVectorGrid3D & renderedGrid, vtkDataArray * dataArray);

    QString name() const override;

    IndexType scalarsAssociation() const override;

    vtkAlgorithmOutput * vectorDataOutputPort() override;

protected:
    /** create an instance for each 3D vector array found in the renderedData */
    static std::vector<std::unique_ptr<GlyphMappingData>> newInstances(RenderedData & renderedData);

protected:
    void updateArrowLength();

private:
    static const bool s_registered;

    vtkSmartPointer<vtkAssignAttribute> m_assignVectors;
    RenderedVectorGrid3D & m_renderedGrid;
    /** The data array here is only the resampled copy of the original data. It can be released by its source algorithm,
      * so keep a reference here */
    vtkSmartPointer<vtkDataArray> m_dataArray;
};
