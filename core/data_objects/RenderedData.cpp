#include "RenderedData.h"

#include <vtkLookupTable.h>
#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkInformationIntegerPointerKey.h>

#include <core/data_objects/DataObject.h>
#include <core/vector_mapping/VectorMapping.h>
#include <core/vector_mapping/VectorMappingData.h>


RenderedData::RenderedData(DataObject * dataObject)
    : QObject()
    , m_dataObject(dataObject)
    , m_scalars(nullptr)
    , m_vectors(nullptr)
    , m_isVisible(true)
{
    connect(dataObject, &DataObject::dataChanged, this, &RenderedData::geometryChanged);
}

RenderedData::~RenderedData()
{
    delete m_vectors;
}

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

bool RenderedData::isVisible() const
{
    return m_isVisible;
}

void RenderedData::setVisible(bool visible)
{
    m_isVisible = visible;

    mainActor()->SetVisibility(visible);

    visibilityChangedEvent(visible);
}

QList<vtkActor *> RenderedData::fetchAttributeActors()
{
    QList<vtkActor *> actors;
    for (auto * v : vectorMapping()->vectors())
        actors << v->actor();

    return actors;
}

void RenderedData::scalarsForColorMappingChangedEvent()
{
}

void RenderedData::colorMappingGradientChangedEvent()
{
}

void RenderedData::vectorsForSurfaceMappingChangedEvent()
{
}

void RenderedData::visibilityChangedEvent(bool visible)
{
    for (VectorMappingData * vectors : m_vectors->vectors())
        vectors->actor()->SetVisibility(
        visible && vectors->isVisible());
}

DataObject * RenderedData::dataObject()
{
    return m_dataObject;
}

const DataObject * RenderedData::dataObject() const
{
    return m_dataObject;
}

VectorMapping * RenderedData::vectorMapping()
{
    if (!m_vectors)
    {
        m_vectors = new VectorMapping(this);
        connect(m_vectors, &VectorMapping::vectorsChanged, this, &RenderedData::attributeActorsChanged);
    }
    return m_vectors;
}

void RenderedData::setScalarsForColorMapping(ScalarsForColorMapping * scalars)
{
    if (scalars == m_scalars)
        return;

    m_scalars = scalars;

    scalarsForColorMappingChangedEvent();
}

void RenderedData::setColorMappingGradient(vtkScalarsToColors * gradient)
{
    if (m_gradient == gradient)
        return;

    m_gradient = gradient;

    colorMappingGradientChangedEvent();
}
