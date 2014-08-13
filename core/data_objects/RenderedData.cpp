#include "RenderedData.h"

#include <vtkLookupTable.h>
#include <vtkProperty.h>
#include <vtkActor.h>


RenderedData::RenderedData(DataObject * dataObject)
    : m_dataObject(dataObject)
    , m_scalars(nullptr)
    , m_lut(nullptr)
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

QList<vtkActor *> RenderedData::actors()
{
    QList<vtkActor *> a;
    a << mainActor();
    a << attributeActors();
    return a;
}

vtkActor * RenderedData::mainActor()
{
    if (!m_actor)
    {
        m_actor.TakeReference(createActor());
        m_actor->SetProperty(renderProperty());
    }

    return m_actor;
}

QList<vtkActor *> RenderedData::attributeActors()
{
    return fetchAttributeActors();
}

QList<vtkActor *> RenderedData::fetchAttributeActors()
{
    return{};
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
    if (scalars == m_scalars)
        return;

    m_scalars = scalars;

    scalarsForColorMappingChangedEvent();
}

void RenderedData::applyGradientLookupTable(vtkLookupTable * gradient)
{
    if (gradient && m_lutMTime == gradient->GetMTime())
        return;

    m_lutMTime = gradient->GetMTime();

    m_lut = gradient;

    gradientForColorMappingChangedEvent();
}
