#include "RenderedData3D.h"

#include <cassert>

#include <vtkActorCollection.h>

#include <vtkProperty.h>

#include <core/types.h>
#include <core/vtkhelper.h>
#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>


RenderedData3D::RenderedData3D(DataObject * dataObject)
    : RenderedData(ContentType::Rendered3D, dataObject)
    , m_vectors(nullptr)
{
}

RenderedData3D::~RenderedData3D()
{
    delete m_vectors;
}

vtkSmartPointer<vtkActorCollection> RenderedData3D::actors()
{
    assert(vtkActorCollection::SafeDownCast(viewProps().Get()));
    return static_cast<vtkActorCollection *>(viewProps().Get());
}

GlyphMapping * RenderedData3D::glyphMapping()
{
    if (!m_vectors)
    {
        m_vectors = new GlyphMapping(this);
        connect(m_vectors, &GlyphMapping::vectorsChanged, this, &RenderedData::viewPropCollectionChanged);
    }
    return m_vectors;
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
    return fetchActors();
}

vtkSmartPointer<vtkActorCollection> RenderedData3D::fetchActors()
{
    VTK_CREATE(vtkActorCollection, actors);

    for (auto * v : glyphMapping()->vectors())
        actors->AddItem(v->actor());

    return actors;
}

void RenderedData3D::visibilityChangedEvent(bool visible)
{
    for (GlyphMappingData * vectors : m_vectors->vectors())
        vectors->viewProp()->SetVisibility(
        visible && vectors->isVisible());
}

void RenderedData3D::vectorsForSurfaceMappingChangedEvent()
{
    invalidateViewProps();
}
