#include "RenderedData.h"

#include <vtkProperty.h>
#include <vtkActor.h>


RenderedData::RenderedData(DataObject * dataObject)
    : m_dataObject(dataObject)
    , m_scalars(nullptr)
    , m_gradient(nullptr)
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

DataObject * RenderedData::dataObject()
{
    return m_dataObject;
}

const DataObject * RenderedData::dataObject() const
{
    return m_dataObject;
}

void RenderedData::applyScalarsForColorMapping(ScalarsForColorMapping * scalars)
{
    m_scalars = scalars;

    updateScalarToColorMapping();
}

void RenderedData::applyColorGradient(const QImage * gradient)
{
    m_gradient = gradient;

    updateScalarToColorMapping();
}
