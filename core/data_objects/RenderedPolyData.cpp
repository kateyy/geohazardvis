#include "RenderedPolyData.h"

#include <cassert>

#include <QImage>

#include <vtkLookupTable.h>

#include <vtkPolyData.h>

#include <vtkElevationFilter.h>

#include <vtkPolyDataMapper.h>

#include <vtkProperty.h>
#include <vtkActor.h>

#include <reflectionzeug/PropertyGroup.h>

#include "PolyDataObject.h"

#include "core/Input.h"
#include "core/vtkhelper.h"
#include "core/data_mapping/ScalarsForColorMapping.h"


using namespace reflectionzeug;


namespace
{
    enum class Interpolation
    {
        flat = VTK_FLAT, gouraud = VTK_GOURAUD, phong = VTK_PHONG
    };
    enum class Representation
    {
        points = VTK_POINTS, wireframe = VTK_WIREFRAME, surface = VTK_SURFACE
    };
}

RenderedPolyData::RenderedPolyData(PolyDataObject * dataObject)
    : RenderedData(dataObject)
{
    m_normalRepresentation.setData(dataObject->polyDataInput()->polyData());
    m_normalRepresentation.setVisible(false);
    connect(&m_normalRepresentation, &NormalRepresentation::geometryChanged, this, &RenderedPolyData::geometryChanged);
}

RenderedPolyData::~RenderedPolyData() = default;

PolyDataObject * RenderedPolyData::polyDataObject()
{
    assert(dynamic_cast<PolyDataObject*>(dataObject()));
    return static_cast<PolyDataObject *>(dataObject());
}

const PolyDataObject * RenderedPolyData::polyDataObject() const
{
    assert(dynamic_cast<const PolyDataObject*>(dataObject()));
    return static_cast<const PolyDataObject *>(dataObject());
}

reflectionzeug::PropertyGroup * RenderedPolyData::createConfigGroup()
{
    PropertyGroup * configGroup = new PropertyGroup();

    auto * renderSettings = new PropertyGroup("renderSettings");
    renderSettings->setTitle("rendering");
    configGroup->addProperty(renderSettings);

    if (renderProperty())
    {
        auto * color = renderSettings->addProperty<Color>("color",
            [this]() {
                double * color = renderProperty()->GetColor();
                return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
            },
            [this](const Color & color) {
                renderProperty()->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
                emit geometryChanged();
        });


        auto * edgesVisible = renderSettings->addProperty<bool>("edgesVisible",
            [this]() {
                return (renderProperty()->GetEdgeVisibility() == 0) ? false : true;
            },
            [this](bool vis) {
                renderProperty()->SetEdgeVisibility(vis);
                emit geometryChanged();
        });
        edgesVisible->setTitle("edge visible");

        auto * lineWidth = renderSettings->addProperty<float>("lineWidth",
            std::bind(&vtkProperty::GetLineWidth, renderProperty()),
            [this](float width) {
                renderProperty()->SetLineWidth(width);
                emit geometryChanged();
        });
        lineWidth->setTitle("line width");
        lineWidth->setMinimum(0.1);
        lineWidth->setMaximum(std::numeric_limits<float>::max());
        lineWidth->setStep(0.1);

        auto * edgeColor = renderSettings->addProperty<Color>("edgeColor",
            [this]() {
                double * color = renderProperty()->GetEdgeColor();
                return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
            },
            [this](const Color & color) {
                renderProperty()->SetEdgeColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
                emit geometryChanged();
        });
        edgeColor->setTitle("edge color");


        auto * representation = renderSettings->addProperty<Representation>("representation",
            [this]() {
                return static_cast<Representation>(renderProperty()->GetRepresentation());
            },
            [this](const Representation & rep) {
                renderProperty()->SetRepresentation(static_cast<int>(rep));
                emit geometryChanged();
        });
        representation->setStrings({
                { Representation::points, "points" },
                { Representation::wireframe, "wireframe" },
                { Representation::surface, "surface" }
        });

        auto * lightingEnabled = renderSettings->addProperty<bool>("lightingEnabled",
            std::bind(&vtkProperty::GetLighting, renderProperty()),
            [this](bool enabled) {
                renderProperty()->SetLighting(enabled);
                emit geometryChanged();
        });
        lightingEnabled->setTitle("lighting enabled");

        auto * interpolation = renderSettings->addProperty<Interpolation>("interpolation",
            [this]() {
                return static_cast<Interpolation>(renderProperty()->GetInterpolation());
            },
            [this](const Interpolation & i) {
                renderProperty()->SetInterpolation(static_cast<int>(i));
                emit geometryChanged();
        });
        interpolation->setTitle("interpolation");
        interpolation->setStrings({
                { Interpolation::flat, "flat" },
                { Interpolation::gouraud, "gouraud" },
                { Interpolation::phong, "phong" }
        });

        auto transparency = renderSettings->addProperty<double>("transparency",
            [this]() {
                return 1.0 - renderProperty()->GetOpacity();
            },
            [this](double transparency) {
                renderProperty()->SetOpacity(1.0 - transparency);
                emit geometryChanged();
        });
        transparency->setMinimum(0.f);
        transparency->setMaximum(1.f);
        transparency->setStep(0.01f);
    }

    configGroup->addProperty(m_normalRepresentation.createPropertyGroup());

    return configGroup;
}

vtkProperty * RenderedPolyData::createDefaultRenderProperty() const
{
    vtkProperty * prop = vtkProperty::New();
    prop->SetColor(0, 0.6, 0);
    prop->SetOpacity(1.0);
    prop->SetInterpolationToFlat();
    prop->SetEdgeVisibility(true);
    prop->SetEdgeColor(0.1, 0.1, 0.1);
    prop->SetLineWidth(1.2);
    prop->SetBackfaceCulling(false);
    prop->SetLighting(false);

    return prop;
}

vtkActor * RenderedPolyData::createActor() const
{
    vtkActor * actor = vtkActor::New();
    actor->SetMapper(createDataMapper());

    return actor;
}

QList<vtkActor *> RenderedPolyData::fetchAttributeActors()
{
    return{ m_normalRepresentation.actor() };
}

void RenderedPolyData::updateScalarToColorMapping()
{
    mainActor()->SetMapper(createDataMapper());
}

vtkPolyDataMapper * RenderedPolyData::createDataMapper() const
{
    const PolyDataInput & input = *polyDataObject()->polyDataInput().get();

    vtkPolyDataMapper * mapper = input.createNamedMapper();

    // no mapping: use default colors
    if (!m_scalars || !m_lut || !m_scalars->usesGradients())
    {
        mapper->SetInputData(input.polyData());
        return mapper;
    }

    VTK_CREATE(vtkElevationFilter, elevation);
    elevation->SetInputData(input.polyData());

    float minValue = m_scalars->minValue();
    float maxValue = m_scalars->maxValue();

    // TODO cleanup
    if (m_scalars->name() == "x values")
    {
        elevation->SetLowPoint(minValue, 0, 0);
        elevation->SetHighPoint(maxValue, 0, 0);
    }
    else if (m_scalars->name() == "y values")
    {
        elevation->SetLowPoint(0, minValue, 0);
        elevation->SetHighPoint(0, maxValue, 0);
    }
    else if (m_scalars->name() == "z values")
    {
        elevation->SetLowPoint(0, 0, minValue);
        elevation->SetHighPoint(0, 0, maxValue);
    }
    else
    {
        assert(false);
    }

    mapper->SetInputConnection(elevation->GetOutputPort());

    m_lut->SetValueRange(minValue, maxValue);

    mapper->SetLookupTable(m_lut);

    return mapper;
}
