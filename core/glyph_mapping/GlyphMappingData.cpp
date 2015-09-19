#include "GlyphMappingData.h"

#include <cassert>
#include <algorithm>

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

#include <core/color_mapping/ColorMappingData.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>


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
    : m_renderedData(renderedData)
    , m_isVisible(false)
    , m_actor(vtkSmartPointer<vtkActor>::New())
    , m_colorMappingData(nullptr)
    , m_isValid(true)
{
    auto lineArrow = vtkSmartPointer<vtkLineSource>::New();
    lineArrow->SetPoint1(0.f, 0.f, 0.f);
    lineArrow->SetPoint2(1.f, 0.f, 0.f);
    m_arrowSources.insert(Representation::Line, lineArrow);

    m_arrowSources.insert(Representation::SimpleArrow, createSimpleArrow());

    auto cylindricArrow = vtkSmartPointer<vtkArrowSource>::New();
    m_arrowSources.insert(Representation::CylindricArrow, cylindricArrow);
    cylindricArrow->SetShaftRadius(0.02);
    cylindricArrow->SetTipRadius(0.07);
    cylindricArrow->SetTipLength(0.3);

    m_arrowGlyph = vtkSmartPointer<vtkGlyph3D>::New();
    m_arrowGlyph->ScalingOn();
    m_arrowGlyph->SetScaleModeToDataScalingOff();
    m_arrowGlyph->OrientOn();
    double * bounds = renderedData.dataObject().dataSet()->GetBounds();
    double maxBoundsSize = std::max(bounds[1] - bounds[0], std::max(bounds[3] - bounds[2], bounds[5] - bounds[4]));
    m_arrowGlyph->SetScaleFactor(maxBoundsSize * 0.1);

    setRepresentation(Representation::CylindricArrow);

    m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    m_mapper->UseLookupTableScalarRangeOn();
    m_mapper->SetInputConnection(m_arrowGlyph->GetOutputPort());
    m_mapper->ScalarVisibilityOff();

    auto & mapperInfo = *m_mapper->GetInformation();
    mapperInfo.Set(DataObject::NameKey(), renderedData.dataObject().name().toUtf8().data());
    DataObject::storePointer(mapperInfo, &renderedData.dataObject());
    AbstractVisualizedData::storePointer(mapperInfo, &renderedData);

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
        return;

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
    vtkArrowSource * arrowSouce = vtkArrowSource::SafeDownCast(m_arrowSources[Representation::CylindricArrow]);
    assert(arrowSouce);

    return (float)arrowSouce->GetTipRadius();
}

void GlyphMappingData::setArrowRadius(float radius)
{
    vtkArrowSource * arrowSouce = vtkArrowSource::SafeDownCast(m_arrowSources[Representation::CylindricArrow]);
    assert(arrowSouce);

    arrowSouce->SetTipRadius(radius);
    arrowSouce->SetShaftRadius(radius * 0.1f);
}

float GlyphMappingData::arrowTipLength() const
{
    vtkArrowSource * arrowSouce = vtkArrowSource::SafeDownCast(m_arrowSources[Representation::CylindricArrow]);
    assert(arrowSouce);

    return (float)arrowSouce->GetTipLength();
}

void GlyphMappingData::setArrowTipLength(float tipLength)
{
    vtkArrowSource * arrowSouce = vtkArrowSource::SafeDownCast(m_arrowSources[Representation::CylindricArrow]);
    assert(arrowSouce);

    arrowSouce->SetTipLength(tipLength);
}

unsigned GlyphMappingData::lineWidth() const
{
    return (unsigned)m_actor->GetProperty()->GetLineWidth();
}

void GlyphMappingData::setLineWidth(unsigned lineWidth)
{
    m_actor->GetProperty()->SetLineWidth(static_cast<float>(lineWidth));
}

reflectionzeug::PropertyGroup * GlyphMappingData::createPropertyGroup()
{
    reflectionzeug::PropertyGroup * group = new reflectionzeug::PropertyGroup();

    auto prop_representation = group->addProperty<Representation>("style",
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

ColorMappingData * GlyphMappingData::colorMappingData()
{
    return m_colorMappingData;
}

void GlyphMappingData::setColorMappingData(ColorMappingData * colorMappingData)
{
    if (m_colorMappingData == colorMappingData)
        return;

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
        return;

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
            colorMappingData->createFilter(&renderedData())->GetOutputPort());

        colorMappingData->configureMapper(&renderedData(), m_mapper);
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
