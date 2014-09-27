#include "RenderedData.h"

#include <vtkLookupTable.h>
#include <vtkProperty.h>
#include <vtkActor.h>

#include <core/data_objects/DataObject.h>
#include <core/vector_mapping/VectorsToSurfaceMapping.h>


RenderedData::RenderedData(DataObject * dataObject)
    : QObject()
    , m_dataObject(dataObject)
    , m_scalars(nullptr)
    , m_lut(nullptr)
    , m_vectors(new VectorsToSurfaceMapping(this))
    , m_isVisible(true)
{
    connect(m_vectors, &VectorsToSurfaceMapping::vectorsChanged, this, &RenderedData::attributeActorsChanged);
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
    return{};
}

void RenderedData::scalarsForColorMappingChangedEvent()
{
}

void RenderedData::gradientForColorMappingChangedEvent()
{
}

void RenderedData::vectorsForSurfaceMappingChangedEvent()
{
}

void RenderedData::visibilityChangedEvent(bool /*visible*/)
{
}

DataObject * RenderedData::dataObject()
{
    return m_dataObject;
}

const DataObject * RenderedData::dataObject() const
{
    return m_dataObject;
}

VectorsToSurfaceMapping * RenderedData::vectorMapping()
{
    return m_vectors;
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
