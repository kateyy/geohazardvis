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

#include "RenderedData3D.h"

#include <cassert>

#include <vtkProp3DCollection.h>
#include <vtkProperty.h>

#include <core/types.h>
#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>


RenderedData3D::RenderedData3D(CoordinateTransformableDataObject & dataObject)
    : RenderedData(ContentType::Rendered3D, dataObject)
    , m_glyphMapping{ nullptr }
{
}

RenderedData3D::~RenderedData3D() = default;

vtkSmartPointer<vtkProp3DCollection> RenderedData3D::viewProps3D()
{
    auto result = viewProps();
    assert(dynamic_cast<vtkProp3DCollection *>(result.Get()));
    return static_cast<vtkProp3DCollection *>(result.Get());
}

GlyphMapping & RenderedData3D::glyphMapping()
{
    if (!m_glyphMapping)
    {
        m_glyphMapping = std::make_unique<GlyphMapping>(*this);
        connect(m_glyphMapping.get(), &GlyphMapping::vectorsChanged, this, &RenderedData3D::invalidateViewProps);
    }
    return *m_glyphMapping;
}

vtkProperty * RenderedData3D::renderProperty()
{
    if (!m_renderProperty)
    {
        m_renderProperty = createDefaultRenderProperty();
    }

    return m_renderProperty;
}

vtkSmartPointer<vtkProperty> RenderedData3D::createDefaultRenderProperty() const
{
    return vtkSmartPointer<vtkProperty>::New();
}

vtkSmartPointer<vtkPropCollection> RenderedData3D::fetchViewProps()
{
    return fetchViewProps3D();
}

vtkSmartPointer<vtkProp3DCollection> RenderedData3D::fetchViewProps3D()
{
    auto props = vtkSmartPointer<vtkProp3DCollection>::New();

    for (auto v : glyphMapping().vectors())
    {
        props->AddItem(v->viewProp3D());
    }

    return props;
}

void RenderedData3D::visibilityChangedEvent(bool visible)
{
    RenderedData::visibilityChangedEvent(visible);

    if (!m_glyphMapping)
    {
        return;
    }

    for (auto vectors : m_glyphMapping->vectors())
    {
        vectors->viewProp()->SetVisibility(
            visible && vectors->isVisible());
    }
}

void RenderedData3D::vectorsForSurfaceMappingChangedEvent()
{
    invalidateViewProps();
}
