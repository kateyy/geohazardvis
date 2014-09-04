#include "VectorsForSurfaceMapping.h"

#include <cassert>

#include <vtkActor.h>


VectorsForSurfaceMapping::VectorsForSurfaceMapping(RenderedData * renderedData)
    : m_renderedData(renderedData)
    , m_isVisible(false)
    , m_actor(vtkSmartPointer<vtkActor>::New())
{
    assert(renderedData);
    m_actor->SetVisibility(m_isVisible);
    m_actor->PickableOff();
}

bool VectorsForSurfaceMapping::isVisible() const
{
    return m_isVisible;
}

void VectorsForSurfaceMapping::setVisible(bool visible)
{
    if (m_isVisible == visible)
        return;

    m_isVisible = visible;
    m_actor->SetVisibility(visible);

    visibilityChangedEvent();
}

vtkActor * VectorsForSurfaceMapping::actor()
{
    return m_actor;
}

void VectorsForSurfaceMapping::initialize()
{
}

void VectorsForSurfaceMapping::visibilityChangedEvent()
{
}

VectorsForSurfaceMapping::~VectorsForSurfaceMapping() = default;
