#include "RenderedData3D.h"

#include <cassert>

#include <vtkProp3DCollection.h>

#include <vtkProperty.h>
#include <vtkActor.h>

#include <core/vtkhelper.h>
#include <core/vector_mapping/VectorMapping.h>
#include <core/vector_mapping/VectorMappingData.h>


RenderedData3D::RenderedData3D(DataObject * dataObject)
    : RenderedData(dataObject)
    , m_vectors(nullptr)
{
}

RenderedData3D::~RenderedData3D()
{
    delete m_vectors;
}

vtkSmartPointer<vtkProp3DCollection> RenderedData3D::viewProps3D()
{
    assert(vtkProp3DCollection::SafeDownCast(viewProps().Get()));
    return static_cast<vtkProp3DCollection *>(viewProps().Get());
}

VectorMapping * RenderedData3D::vectorMapping()
{
    if (!m_vectors)
    {
        m_vectors = new VectorMapping(this);
        connect(m_vectors, &VectorMapping::vectorsChanged, this, &RenderedData::viewPropCollectionChanged);
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
    return fetchViewProps3D();
}

vtkSmartPointer<vtkProp3DCollection> RenderedData3D::fetchViewProps3D()
{
    VTK_CREATE(vtkProp3DCollection, props);

    for (auto * v : vectorMapping()->vectors())
        props->AddItem(v->actor());

    return props;
}

void RenderedData3D::visibilityChangedEvent(bool visible)
{
    for (VectorMappingData * vectors : m_vectors->vectors())
        vectors->viewProp()->SetVisibility(
        visible && vectors->isVisible());
}

void RenderedData3D::vectorsForSurfaceMappingChangedEvent()
{
    invalidateViewProps();
}
