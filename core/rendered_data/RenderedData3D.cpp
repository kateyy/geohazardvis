#include "RenderedData3D.h"

#include <cassert>

#include <vtkProp3DCollection.h>
#include <vtkProperty.h>

#include <core/types.h>
#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>


RenderedData3D::RenderedData3D(DataObject & dataObject)
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

    for (auto * v : glyphMapping().vectors())
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

    for (GlyphMappingData * vectors : m_glyphMapping->vectors())
    {
        vectors->viewProp()->SetVisibility(
            visible && vectors->isVisible());
    }
}

void RenderedData3D::vectorsForSurfaceMappingChangedEvent()
{
    invalidateViewProps();
}
