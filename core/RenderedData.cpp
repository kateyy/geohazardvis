#include "RenderedData.h"

#include <vtkProperty.h>
#include <vtkActor.h>


RenderedData::RenderedData(std::shared_ptr<const DataObject> dataObject)
    : m_dataObject(dataObject)
{
}

RenderedData::~RenderedData() = default;

vtkProperty * RenderedData::renderProperty()
{
    if (!m_renderProperty)
    {
        m_renderProperty.TakeReference(createDefaultRenderProperty());
    }

    return m_renderProperty;
}

vtkActor * RenderedData::actor()
{
    if (!m_actor)
    {
        m_actor.TakeReference(createActor());
        m_actor->SetProperty(renderProperty());
    }

    return m_actor;
}

std::shared_ptr<const DataObject> RenderedData::dataObject() const
{
    return m_dataObject;
}
