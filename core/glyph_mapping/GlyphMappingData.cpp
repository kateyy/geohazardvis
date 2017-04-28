#include "GlyphMappingData.h"

#include <algorithm>
#include <cassert>
#include <limits>

#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkArrowSource.h>
#include <vtkAssignAttribute.h>
#include <vtkGlyph3D.h>
#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkLineSource.h>
#include <vtkLookupTable.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkVectorNorm.h>

#include <reflectionzeug/Property.h>
#include <reflectionzeug/PropertyGroup.h>

#include <core/OpenGLDriverFeatures.h>
#include <core/color_mapping/ColorMappingData.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>


using namespace reflectionzeug;

namespace
{

vtkSmartPointer<vtkAlgorithm> createSimpleArrow()
{
    auto shaft = vtkSmartPointer<vtkLineSource>::New();
    shaft->SetPoint1(0.f, 0.f, 0.f);
    shaft->SetPoint2(1.f, 0.f, 0.f);

    auto cone1 = vtkSmartPointer<vtkLineSource>::New();
    cone1->SetPoint1(1.00f, 0.0f, 0.f);
    cone1->SetPoint2(0.65f, 0.1f, 0.f);

    auto cone2 = vtkSmartPointer<vtkLineSource>::New();
    cone2->SetPoint1(1.00f, 0.0f, 0.f);
    cone2->SetPoint2(0.65f, -0.1f, 0.f);

    auto arrow = vtkSmartPointer<vtkAppendPolyData>::New();
    arrow->AddInputConnection(shaft->GetOutputPort());
    arrow->AddInputConnection(cone1->GetOutputPort());
    arrow->AddInputConnection(cone2->GetOutputPort());

    return arrow;
}

}

GlyphMappingData::GlyphMappingData(RenderedData & renderedData)
    : m_renderedData{ renderedData }
    , m_isVisible{ false }
    , m_actor{ vtkSmartPointer<vtkActor>::New() }
    , m_colorMappingData{ nullptr }
    , m_isValid{ true }
{
    auto lineArrow = vtkSmartPointer<vtkLineSource>::New();
    lineArrow->SetPoint1(0.f, 0.f, 0.f);
    lineArrow->SetPoint2(1.f, 0.f, 0.f);
    m_arrowSources.emplace(Representation::Line, lineArrow);

    m_arrowSources.emplace(Representation::SimpleArrow, createSimpleArrow());

    auto cylindricArrow = vtkSmartPointer<vtkArrowSource>::New();
    m_arrowSources.emplace(Representation::CylindricArrow, cylindricArrow);
    cylindricArrow->SetShaftRadius(0.02);
    cylindricArrow->SetTipRadius(0.07);
    cylindricArrow->SetTipLength(0.3);

    m_arrowGlyph = vtkSmartPointer<vtkGlyph3D>::New();
    m_arrowGlyph->ScalingOn();
    m_arrowGlyph->SetScaleModeToDataScalingOff();
    m_arrowGlyph->OrientOn();
    DataBounds bounds;
    renderedData.dataObject().processedOutputDataSet()->GetBounds(bounds.data());
    const double maxBoundsSize = maxComponent(bounds.componentSize());
    m_arrowGlyph->SetScaleFactor(maxBoundsSize * 0.1);

    setRepresentation(Representation::CylindricArrow);

    m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    m_mapper->UseLookupTableScalarRangeOn();
    m_mapper->SetInputConnection(m_arrowGlyph->GetOutputPort());
    m_mapper->ScalarVisibilityOff();

    AbstractVisualizedData::setupInformation(*m_mapper->GetInformation(), renderedData);

    m_actor->SetVisibility(m_isVisible);
    m_actor->PickableOff();
    m_actor->SetMapper(m_mapper);
}

bool GlyphMappingData::isVisible() const
{
    return m_isVisible;
}

void GlyphMappingData::setVisible(bool visible)
{
    if (m_isVisible == visible)
    {
        return;
    }

    m_isVisible = visible;
    m_actor->SetVisibility(visible);

    visibilityChangedEvent();

    emit visibilityChanged(visible);
}

GlyphMappingData::Representation GlyphMappingData::representation() const
{
    return m_representation;
}

void GlyphMappingData::setRepresentation(Representation representation)
{
    m_representation = representation;

    arrowGlyph()->SetSourceConnection(
        m_arrowSources[representation]->GetOutputPort());
}

const double * GlyphMappingData::color() const
{
    return m_actor->GetProperty()->GetColor();
}

void GlyphMappingData::color(double color[3]) const
{
    m_actor->GetProperty()->GetColor(color);
}

void GlyphMappingData::setColor(double r, double g, double b)
{
    m_actor->GetProperty()->SetColor(r, g, b);
}

float GlyphMappingData::arrowLength() const
{
    return static_cast<float>(m_arrowGlyph->GetScaleFactor());
}

void GlyphMappingData::setArrowLength(float length)
{
    m_arrowGlyph->SetScaleFactor(length);
}

float GlyphMappingData::arrowRadius() const
{
    auto arrowSouce = vtkArrowSource::SafeDownCast(m_arrowSources.at(Representation::CylindricArrow));
    assert(arrowSouce);

    return static_cast<float>(arrowSouce->GetTipRadius());
}

void GlyphMappingData::setArrowRadius(float radius)
{
    auto arrowSouce = vtkArrowSource::SafeDownCast(m_arrowSources.at(Representation::CylindricArrow));
    assert(arrowSouce);

    arrowSouce->SetTipRadius(radius);
    arrowSouce->SetShaftRadius(radius * 0.1f);
}

float GlyphMappingData::arrowTipLength() const
{
    auto arrowSouce = vtkArrowSource::SafeDownCast(m_arrowSources.at(Representation::CylindricArrow));
    assert(arrowSouce);

    return static_cast<float>(arrowSouce->GetTipLength());
}

void GlyphMappingData::setArrowTipLength(float tipLength)
{
    auto arrowSouce = vtkArrowSource::SafeDownCast(m_arrowSources.at(Representation::CylindricArrow));
    assert(arrowSouce);

    arrowSouce->SetTipLength(tipLength);
}

unsigned GlyphMappingData::lineWidth() const
{
    return static_cast<unsigned>(m_actor->GetProperty()->GetLineWidth());
}

void GlyphMappingData::setLineWidth(unsigned lineWidth)
{
    m_actor->GetProperty()->SetLineWidth(static_cast<float>(lineWidth));
}

std::unique_ptr<PropertyGroup> GlyphMappingData::createPropertyGroup()
{
    auto group = std::make_unique<PropertyGroup>();

    group->addProperty<Representation>("Style",
        [this] () {return representation(); },
        [this] (Representation repr) {
        setRepresentation(repr);
        emit geometryChanged();
    })->setStrings({
            { Representation::Line, "Line" },
            { Representation::SimpleArrow, "Arrow (Lines)" },
            { Representation::CylindricArrow, "Arrow (Cylindric)" }
    });

    group->addProperty<Color>("Color",
        [this] () {
        double values[3];
        color(values);
        return Color(static_cast<int>(values[0] * 255), static_cast<int>(values[1] * 255), static_cast<int>(values[2] * 255));
    },
        [this] (const Color & color) {
        setColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    });

    group->addProperty<float>("length",
        [this] () { return arrowLength(); },
        [this] (float length) {
        setArrowLength(length);
        emit geometryChanged();
    })->setOptions({
        { "title", "Glyph Length" },
        { "minimum", -1.e6},
        { "maximum", 1.e6},
        { "step", 0.1f }
    });

    group->addProperty<float>("radius",
        [this] () { return arrowRadius(); },
        [this] (float radius) {
        setArrowRadius(radius);
        emit geometryChanged();
    })->setOptions({
        { "title", "Tip Radius" },
        { "minimum", 0.00001f },
        { "step", 0.01f }
    });

    group->addProperty<float>("tipLength",
        [this] () { return arrowTipLength(); },
        [this] (float tipLength) {
        setArrowTipLength(tipLength);
        emit geometryChanged();
    })->setOptions({
        { "title", "Tip Length" },
        { "minimum", 0.00001f },
        { "maximum", 1.f },
        { "step", 0.01f }
    });

    group->addProperty<unsigned>("lineWidth",
        [this] () { return lineWidth(); },
        [this] (unsigned lineWidth) {
        setLineWidth(lineWidth);
        emit geometryChanged();
    })->setOptions({
        { "title", "Line Width" },
        { "minimum", 1u },
        { "maximum", OpenGLDriverFeatures::clampToMaxSupportedLineWidth(100u) },
        { "step", 1u }
    });

    return group;
}

ColorMappingData * GlyphMappingData::colorMappingData()
{
    return m_colorMappingData;
}

void GlyphMappingData::setColorMappingData(ColorMappingData * colorMappingData)
{
    if (m_colorMappingData == colorMappingData)
    {
        return;
    }

    m_colorMappingData = colorMappingData;

    colorMappingChangedEvent(colorMappingData);
}

vtkScalarsToColors * GlyphMappingData::colorMappingGradient()
{
    return m_colorMappingGradient;
}

void GlyphMappingData::setColorMappingGradient(vtkScalarsToColors * gradient)
{
    if (m_colorMappingGradient == gradient)
    {
        return;
    }

    m_colorMappingGradient = gradient;

    colorMappingGradientChangedEvent(gradient);
}

vtkActor * GlyphMappingData::actor()
{
    return m_actor;
}

vtkProp * GlyphMappingData::viewProp()
{
    return actor();
}

vtkProp3D * GlyphMappingData::viewProp3D()
{
    return actor();
}

void GlyphMappingData::initialize()
{
    m_arrowGlyph->SetInputConnection(vectorDataOutputPort());
}

DataObject & GlyphMappingData::dataObject()
{
    return m_renderedData.dataObject();
}

RenderedData & GlyphMappingData::renderedData()
{
    return m_renderedData;
}

bool GlyphMappingData::isValid() const
{
    return m_isValid;
}

vtkGlyph3D * GlyphMappingData::arrowGlyph()
{
    return m_arrowGlyph;
}

void GlyphMappingData::visibilityChangedEvent()
{
}

void GlyphMappingData::colorMappingChangedEvent(ColorMappingData * colorMappingData)
{
    if (colorMappingData && colorMappingData->usesFilter())
    {
        m_arrowGlyph->SetInputConnection(
            colorMappingData->createFilter(renderedData())->GetOutputPort());

        colorMappingData->configureMapper(renderedData(), *m_mapper);
    }
    else
    {
        m_arrowGlyph->SetInputConnection(vectorDataOutputPort());
        m_mapper->ScalarVisibilityOff();
    }
}

void GlyphMappingData::colorMappingGradientChangedEvent(vtkScalarsToColors * gradient)
{
    m_mapper->SetLookupTable(gradient);
}
