#include "RenderedData3D.h"

#include <cassert>

#include <vtkProp3DCollection.h>
#include <vtkProperty.h>
#include <vtkActor.h>

#include <core/types.h>
#include <core/vtkhelper.h>
#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>


RenderedData3D::RenderedData3D(DataObject * dataObject)
    : RenderedData(ContentType::Rendered3D, dataObject)
    , m_glyphMapping(nullptr)
{
}

RenderedData3D::~RenderedData3D()
{
    delete m_glyphMapping;
}

vtkSmartPointer<vtkProp3DCollection> RenderedData3D::viewProps3D()
{
    assert(vtkProp3DCollection::SafeDownCast(viewProps().Get()));
    return static_cast<vtkProp3DCollection *>(viewProps().Get());
}

GlyphMapping * RenderedData3D::glyphMapping()
{
    if (!m_glyphMapping)
    {
        m_glyphMapping = new GlyphMapping(this);
        connect(m_glyphMapping, &GlyphMapping::vectorsChanged, this, &RenderedData::viewPropCollectionChanged);
    }
    return m_glyphMapping;
}

vtkProperty * RenderedData3D::renderProperty()
{
    if (!m_renderProperty)
        m_renderProperty.TakeReference(createDefaultRenderProperty());

    return m_renderProperty;
}

vtkProperty * RenderedData3D::createDefaultRenderProperty() const
{
    return vtkProperty::New();
}

vtkSmartPointer<vtkPropCollection> RenderedData3D::fetchViewProps()
{
    return fetchViewProps3D();
}

vtkSmartPointer<vtkProp3DCollection> RenderedData3D::fetchViewProps3D()
{
    VTK_CREATE(vtkProp3DCollection, props);

    for (auto * v : glyphMapping()->vectors())
        props->AddItem(v->actor());

    return props;
}

void RenderedData3D::visibilityChangedEvent(bool visible)
{
    if (!m_glyphMapping)
        return;

    for (GlyphMappingData * vectors : m_glyphMapping->vectors())
        vectors->viewProp()->SetVisibility(
        visible && vectors->isVisible());
}

void RenderedData3D::vectorsForSurfaceMappingChangedEvent()
{
    invalidateViewProps();
}
