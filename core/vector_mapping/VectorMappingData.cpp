#include "VectorMappingData.h"

#include <cassert>
#include <algorithm>

#include <QColor>

#include <vtkArrowSource.h>
#include <vtkLineSource.h>
#include <vtkAppendPolyData.h>

#include <vtkGlyph3D.h>
#include <vtkPolyData.h>

#include <vtkPolyDataMapper.h>

#include <vtkActor.h>
#include <vtkProperty.h>

#include <reflectionzeug/Property.h>
#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>


namespace
{

vtkSmartPointer<vtkAlgorithm> createSimpleArrow()
{
    VTK_CREATE(vtkLineSource, shaft);
    shaft->SetPoint1(0.f, 0.f, 0.f);
    shaft->SetPoint2(1.f, 0.f, 0.f);

    VTK_CREATE(vtkLineSource, cone1);
    cone1->SetPoint1(1.00f, 0.0f, 0.f);
    cone1->SetPoint2(0.65f, 0.1f, 0.f);

    VTK_CREATE(vtkLineSource, cone2);
    cone2->SetPoint1(1.00f, 0.0f, 0.f);
    cone2->SetPoint2(0.65f, -0.1f, 0.f);

    VTK_CREATE(vtkAppendPolyData, arrow);
    arrow->AddInputConnection(shaft->GetOutputPort());
    arrow->AddInputConnection(cone1->GetOutputPort());
    arrow->AddInputConnection(cone2->GetOutputPort());

    return arrow;
}

}

VectorMappingData::VectorMappingData(RenderedData * renderedData)
    : m_renderedData(renderedData)
    , m_isVisible(false)
    , m_startingIndex(0ll)
    , m_actor(vtkSmartPointer<vtkActor>::New())
    , m_isValid(true)
{
    assert(renderedData);

    if (!m_isValid)
        return;

    VTK_CREATE(vtkLineSource, lineArrow);
    m_arrowSources.insert(Representation::Line, lineArrow);
    
    m_arrowSources.insert(Representation::SimpleArrow, createSimpleArrow());

    VTK_CREATE(vtkArrowSource, cylindricArrow);
    m_arrowSources.insert(Representation::CylindricArrow, cylindricArrow);
    cylindricArrow->SetShaftRadius(0.02);
    cylindricArrow->SetTipRadius(0.07);
    cylindricArrow->SetTipLength(0.3);

    m_arrowGlyph = vtkSmartPointer<vtkGlyph3D>::New();
    m_arrowGlyph->ScalingOn();
    m_arrowGlyph->SetScaleModeToScaleByScalar();
    m_arrowGlyph->OrientOn();
    double * bounds = renderedData->dataObject()->dataSet()->GetBounds();
    double maxBoundsSize = std::max(bounds[1] - bounds[0], std::max(bounds[3] - bounds[2], bounds[5] - bounds[4]));
    m_arrowGlyph->SetScaleFactor(maxBoundsSize * 0.1);

    setRepresentation(Representation::CylindricArrow);

    m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
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

VectorMappingData::Representation VectorMappingData::representation() const
{
    return m_representation;
}

void VectorMappingData::setRepresentation(Representation representation)
{
    m_representation = representation;

    arrowGlyph()->SetSourceConnection(
        m_arrowSources[representation]->GetOutputPort());
}

const double * VectorMappingData::color() const
{
    return m_actor->GetProperty()->GetColor();
}

void VectorMappingData::color(double color[3]) const
{
    m_actor->GetProperty()->GetColor(color);
}

void VectorMappingData::setColor(double r, double g, double b)
{
    m_actor->GetProperty()->SetColor(r, g, b);
}

float VectorMappingData::arrowLength() const
{
    return static_cast<float>(m_arrowGlyph->GetScaleFactor());
}

void VectorMappingData::setArrowLength(float length)
{
    m_arrowGlyph->SetScaleFactor(length);
}

float VectorMappingData::arrowRadius() const
{
    vtkArrowSource * arrowSouce = vtkArrowSource::SafeDownCast(m_arrowSources[Representation::CylindricArrow]);
    assert(arrowSouce);

    return (float)arrowSouce->GetTipRadius();
}

void VectorMappingData::setArrowRadius(float radius)
{
    vtkArrowSource * arrowSouce = vtkArrowSource::SafeDownCast(m_arrowSources[Representation::CylindricArrow]);
    assert(arrowSouce);

    arrowSouce->SetTipRadius(radius);
    arrowSouce->SetShaftRadius(radius * 0.1f);
}

float VectorMappingData::arrowTipLength() const
{
    vtkArrowSource * arrowSouce = vtkArrowSource::SafeDownCast(m_arrowSources[Representation::CylindricArrow]);
    assert(arrowSouce);

    return (float)arrowSouce->GetTipLength();
}

void VectorMappingData::setArrowTipLength(float tipLength)
{
    vtkArrowSource * arrowSouce = vtkArrowSource::SafeDownCast(m_arrowSources[Representation::CylindricArrow]);
    assert(arrowSouce);

    arrowSouce->SetTipLength(tipLength);
}

unsigned VectorMappingData::lineWidth() const
{
    return (unsigned)m_actor->GetProperty()->GetLineWidth();
}

void VectorMappingData::setLineWidth(unsigned lineWidth)
{
    m_actor->GetProperty()->SetLineWidth(lineWidth);
}

reflectionzeug::PropertyGroup * VectorMappingData::createPropertyGroup()
{
    reflectionzeug::PropertyGroup * group = new reflectionzeug::PropertyGroup();

    auto prop_representation = group->addProperty<Representation>("representation",
        [this] () {return representation(); },
        [this] (Representation repr) {
        setRepresentation(repr);
        emit geometryChanged();
    });
    prop_representation->setStrings({
            { Representation::Line, "line" },
            { Representation::SimpleArrow, "arrow (lines)" },
            { Representation::CylindricArrow, "arrow (cylindric)" }
    });

    auto * edgeColor = group->addProperty<reflectionzeug::Color>("color",
        [this] () {
        double values[3];
        color(values);
        return reflectionzeug::Color(static_cast<int>(values[0] * 255), static_cast<int>(values[1] * 255), static_cast<int>(values[2] * 255));
    },
        [this] (const reflectionzeug::Color & color) {
        setColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    });
    edgeColor->setOption("title", "color");

    auto prop_length = group->addProperty<float>("length",
        [this] () { return arrowLength(); },
        [this] (float length) {
        setArrowLength(length);
        emit geometryChanged();
    });
    prop_length->setOption("title", "arrow length");
    prop_length->setOption("minimum", 0.00001f);
    prop_length->setOption("step", 0.1f);

    auto prop_radius = group->addProperty<float>("radius",
        [this] () { return arrowRadius(); },
        [this] (float radius) {
        setArrowRadius(radius);
        emit geometryChanged();
    });
    prop_radius->setOption("title", "tip radius");
    prop_radius->setOption("minimum", 0.00001f);
    prop_radius->setOption("step", 0.01f);

    auto prop_tipLength = group->addProperty<float>("tipLength",
        [this] () { return arrowTipLength(); },
        [this] (float tipLength) {
        setArrowTipLength(tipLength);
        emit geometryChanged();
    });
    prop_tipLength->setOption("title", "tip length");
    prop_tipLength->setOption("minimum", 0.00001f);
    prop_tipLength->setOption("maximum", 1.f);
    prop_tipLength->setOption("step", 0.01f);

    auto prop_lineWidth = group->addProperty<unsigned>("lineWidth",
        [this] () { return lineWidth(); },
        [this] (unsigned lineWidth) {
        setLineWidth(lineWidth);
        emit geometryChanged();
    });
    prop_lineWidth->setOption("title", "line width");
    prop_lineWidth->setOption("minimum", 1);
    prop_lineWidth->setOption("maximum", 100);
    prop_lineWidth->setOption("step", 1);

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
