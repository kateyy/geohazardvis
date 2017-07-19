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

#include <memory>

#include <core/rendered_data/RenderedData.h>


class vtkProp3DCollection;
class vtkProperty;

class GlyphMapping;


/**
Base class for rendered data represented as vtkActors.
*/
class CORE_API RenderedData3D : public RenderedData
{
public:
    explicit RenderedData3D(CoordinateTransformableDataObject & dataObject);
    ~RenderedData3D() override;

    /** VTK 3D view props visualizing the data object and possibly additional attributes */
    vtkSmartPointer<vtkProp3DCollection> viewProps3D();

    vtkProperty * renderProperty();
    virtual vtkSmartPointer<vtkProperty> createDefaultRenderProperty() const;

    GlyphMapping & glyphMapping();

protected:
    virtual vtkSmartPointer<vtkProp3DCollection> fetchViewProps3D();

    void visibilityChangedEvent(bool visible) override;
    virtual void vectorsForSurfaceMappingChangedEvent();

private:
    /** subclasses are supposed to provide 3D view props (vtkProp3D) */
    vtkSmartPointer<vtkPropCollection> fetchViewProps() override final;

private:
    vtkSmartPointer<vtkProperty> m_renderProperty;

    std::unique_ptr<GlyphMapping> m_glyphMapping;

private:
    Q_DISABLE_COPY(RenderedData3D)
};
