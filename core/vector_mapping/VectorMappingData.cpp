#include "VectorMappingData.h"

#include <cassert>
#include <algorithm>

#include <vtkArrowSource.h>

#include <vtkGlyph3D.h>
#include <vtkPolyData.h>

#include <vtkDataSetMapper.h>

#include <vtkActor.h>
#include <vtkProperty.h>

#include <reflectionzeug/Property.h>
#include <reflectionzeug/PropertyGroup.h>

#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>


VectorMappingData::VectorMappingData(RenderedData * renderedData)
    : m_renderedData(renderedData)
    , m_isVisible(false)
    , m_startingIndex(0ll)
    , m_polyData(vtkPolyData::SafeDownCast(renderedData->dataObject()->dataSet()))
    , m_actor(vtkSmartPointer<vtkActor>::New())
    , m_isValid(m_polyData && (m_polyData->GetCellType(0) == VTK_TRIANGLE))
{
    assert(renderedData);

    if (!m_isValid)
        return;

    m_arrowSource = vtkSmartPointer<vtkArrowSource>::New();
    m_arrowSource->SetShaftRadius(0.02);
    m_arrowSource->SetTipRadius(0.07);
    m_arrowSource->SetTipLength(0.3);

    m_arrowGlyph = vtkSmartPointer<vtkGlyph3D>::New();
    m_arrowGlyph->SetScaleModeToScaleByScalar();
    m_arrowGlyph->OrientOn();
    double * bounds = m_polyData->GetBounds();
    double maxBoundsSize = std::max(bounds[1] - bounds[0], std::max(bounds[3] - bounds[2], bounds[5] - bounds[4]));
    m_arrowGlyph->SetScaleFactor(maxBoundsSize * 0.1);
    m_arrowGlyph->SetSourceConnection(m_arrowSource->GetOutputPort());


    m_mapper = vtkSmartPointer<vtkDataSetMapper>::New();
    m_mapper->SetInputConnection(m_arrowGlyph->GetOutputPort());

    m_actor->SetVisibility(m_isVisible);
    m_actor->PickableOff();
    m_actor->SetMapper(m_mapper);
}

VectorMappingData::~VectorMappingData() = default;

bool VectorMappingData::isVisible() const
{
    return m_isVisible;
}

void VectorMappingData::setVisible(bool visible)
{
    if (m_isVisible == visible)
        return;

    m_isVisible = visible;
    m_actor->SetVisibility(visible);

    visibilityChangedEvent();
}

vtkIdType VectorMappingData::maximumStartingIndex()
{
    return 0;
}

vtkIdType VectorMappingData::startingIndex() const
{
    assert(m_startingIndex <= const_cast<VectorMappingData *>(this)->maximumStartingIndex());
    return m_startingIndex;
}

void VectorMappingData::setStartingIndex(vtkIdType index)
{
    vtkIdType newIndex = std::max(0ll, std::min(index, maximumStartingIndex()));

    if (newIndex == m_startingIndex)
        return;

    m_startingIndex = newIndex;

    startingIndexChangedEvent();
}

float VectorMappingData::arrowLength() const
{
    return static_cast<float>(m_arrowGlyph->GetScaleFactor());
}

void VectorMappingData::setArrowLength(float length)
{
    m_arrowGlyph->SetScaleFactor(length);

    emit geometryChanged();
}

float VectorMappingData::arrowRadius() const
{
    return (float)m_arrowSource->GetTipRadius();
}

void VectorMappingData::setArrowRadius(float radius)
{
    m_arrowSource->SetTipRadius(radius);
    m_arrowSource->SetShaftRadius(radius * 0.1f);

    emit geometryChanged();
}

float VectorMappingData::arrowTipLength() const
{
    return (float)m_arrowSource->GetTipLength();
}

void VectorMappingData::setArrowTipLength(float tipLength)
{
    m_arrowSource->SetTipLength(tipLength);

    emit geometryChanged();
}

reflectionzeug::PropertyGroup * VectorMappingData::createPropertyGroup()
{
    reflectionzeug::PropertyGroup * group = new reflectionzeug::PropertyGroup();

    auto prop_length = group->addProperty<float>("length", this,
        &VectorMappingData::arrowLength, &VectorMappingData::setArrowLength);
    prop_length->setOption("title", "arrow length");
    prop_length->setOption("minimum", 0.00001f);
    prop_length->setOption("step", 0.1f);

    auto prop_radius = group->addProperty<float>("radius", this,
        &VectorMappingData::arrowRadius, &VectorMappingData::setArrowRadius);
    prop_radius->setOption("title", "tip radius");
    prop_radius->setOption("minimum", 0.00001f);
    prop_radius->setOption("step", 0.01f);

    auto prop_tipLength = group->addProperty<float>("tipLength", this,
        &VectorMappingData::arrowTipLength, &VectorMappingData::setArrowTipLength);
    prop_tipLength->setOption("title", "tip length");
    prop_tipLength->setOption("minimum", 0.00001f);
    prop_tipLength->setOption("maximum", 1.f);
    prop_tipLength->setOption("step", 0.01f);

    auto * edgeColor = group->addProperty<reflectionzeug::Color>("color",
        [this]() {
        double * color = actor()->GetProperty()->GetColor();
        return reflectionzeug::Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [this](const reflectionzeug::Color & color) {
        actor()->GetProperty()->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    });
    edgeColor->setOption("title", "color");

    return group;
}

vtkActor * VectorMappingData::actor()
{
    return m_actor;
}

void VectorMappingData::initialize()
{
}

DataObject * VectorMappingData::dataObject()
{
    return m_renderedData->dataObject();
}

RenderedData * VectorMappingData::renderedData()
{
    return m_renderedData;
}

bool VectorMappingData::isValid() const
{
    return m_isValid;
}

vtkPolyData * VectorMappingData::polyData()
{
    return m_polyData;
}

vtkGlyph3D * VectorMappingData::arrowGlyph()
{
    return m_arrowGlyph;
}

void VectorMappingData::visibilityChangedEvent()
{
}

void VectorMappingData::startingIndexChangedEvent()
{
}
