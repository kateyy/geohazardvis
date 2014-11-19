#include "RenderedPolyData.h"

#include <cassert>

#include <QImage>

#include <vtkInformation.h>
#include <vtkInformationIntegerPointerKey.h>
#include <vtkInformationStringKey.h>

#include <vtkPolyData.h>

#include <vtkPolyDataMapper.h>

#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkActorCollection.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/scalar_mapping/ScalarsForColorMapping.h>


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
    : RenderedData3D(dataObject)
    , m_mapper(vtkSmartPointer<vtkPolyDataMapper>::New())
    , m_mainActor(vtkSmartPointer<vtkActor>::New())
{
    assert(vtkPolyData::SafeDownCast(dataObject->dataSet()));

    vtkInformation * mapperInfo = m_mapper->GetInformation();
    mapperInfo->Set(DataObject::NameKey(), dataObject->name().toLatin1().data());
    DataObject::setDataObject(mapperInfo, dataObject);

    // don't break the lut configuration
    m_mapper->UseLookupTableScalarRangeOn();

    m_mainActor->SetMapper(m_mapper);
    m_mainActor->SetProperty(renderProperty());
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
    PropertyGroup * renderSettings = new PropertyGroup();

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
    edgesVisible->setOption("title", "edge visible");

    auto * lineWidth = renderSettings->addProperty<unsigned>("lineWidth",
        [this] () {
        return static_cast<unsigned>(renderProperty()->GetLineWidth());
    },
        [this] (unsigned width) {
        renderProperty()->SetLineWidth(width);
        emit geometryChanged();
    });
    lineWidth->setOption("title", "line width");
    lineWidth->setOption("minimum", 1);
    lineWidth->setOption("maximum", std::numeric_limits<unsigned>::max());
    lineWidth->setOption("step", 1);
    lineWidth->setOption("suffix", " pixel");

    auto * edgeColor = renderSettings->addProperty<Color>("edgeColor",
        [this]() {
        double * color = renderProperty()->GetEdgeColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [this](const Color & color) {
        renderProperty()->SetEdgeColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    });
    edgeColor->setOption("title", "edge color");


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
    lightingEnabled->setOption("title", "lighting enabled");

    auto * interpolation = renderSettings->addProperty<Interpolation>("interpolation",
        [this]() {
        return static_cast<Interpolation>(renderProperty()->GetInterpolation());
    },
        [this](const Interpolation & i) {
        renderProperty()->SetInterpolation(static_cast<int>(i));
        emit geometryChanged();
    });
    interpolation->setOption("title", "interpolation");
    interpolation->setStrings({
            { Interpolation::flat, "flat" },
            { Interpolation::gouraud, "gouraud" },
            { Interpolation::phong, "phong" }
    });

    auto transparency = renderSettings->addProperty<double>("transparency",
        [this]() {
        return (1.0 - renderProperty()->GetOpacity()) * 100;
    },
        [this](double transparency) {
        renderProperty()->SetOpacity(1.0 - transparency * 0.01);
        emit geometryChanged();
    });
    transparency->setOption("minimum", 0);
    transparency->setOption("maximum", 100);
    transparency->setOption("step", 1);
    transparency->setOption("suffix", " %");

    auto pointSize = renderSettings->addProperty<unsigned>("pointSize",
        [this]() {
        return static_cast<unsigned>(renderProperty()->GetPointSize());
    },
        [this](unsigned pointSize) {
        renderProperty()->SetPointSize(pointSize);
        emit geometryChanged();
    });
    pointSize->setOption("title", "point size");
    pointSize->setOption("minimum", 1);
    pointSize->setOption("maximum", 20);
    pointSize->setOption("step", 1);
    pointSize->setOption("suffix", " pixel");

    return renderSettings;
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

vtkSmartPointer<vtkActorCollection> RenderedPolyData::fetchActors()
{
    vtkSmartPointer<vtkActorCollection> actors = RenderedData3D::fetchActors();
    actors->AddItem(m_mainActor);

    return actors;
}

void RenderedPolyData::scalarsForColorMappingChangedEvent()
{
    // no mapping yet, so just render the data set
    if (!m_scalars)
    {
        m_mapper->SetInputConnection(dataObject()->processedOutputPort());
        return;
    }

    m_scalars->configureDataObjectAndMapper(dataObject(), m_mapper);

    if (m_scalars->usesFilter())
    {
        vtkSmartPointer<vtkAlgorithm> filter = vtkSmartPointer<vtkAlgorithm>::Take(m_scalars->createFilter(dataObject()));
        m_mapper->SetInputConnection(filter->GetOutputPort());
    }
    else
        m_mapper->SetInputConnection(dataObject()->processedOutputPort());
}

void RenderedPolyData::colorMappingGradientChangedEvent()
{
    m_mapper->SetLookupTable(m_gradient);
}

void RenderedPolyData::visibilityChangedEvent(bool visible)
{
    RenderedData3D::visibilityChangedEvent(visible);

    m_mainActor->SetVisibility(visible);
}