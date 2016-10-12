#include "RenderedPointCloudData.h"

#include <cassert>

#include <QDebug>

#include <vtkActor.h>
#include <vtkAlgorithmOutput.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProp3DCollection.h>
#include <vtkProperty.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/data_objects/PointCloudDataObject.h>
#include <core/color_mapping/ColorMappingData.h>
#include <core/utility/DataExtent.h>


using namespace reflectionzeug;


RenderedPointCloudData::RenderedPointCloudData(PointCloudDataObject & dataObject)
    : RenderedData3D(dataObject)
    , m_mapper{ vtkSmartPointer<vtkPolyDataMapper>::New() }
{
    setupInformation(*m_mapper->GetInformation(), *this);

    // don't break the lut configuration
    m_mapper->UseLookupTableScalarRangeOn();

    /** Changing vertex/index data may result in changed bounds */
    connect(this, &AbstractVisualizedData::geometryChanged, this, &RenderedPointCloudData::invalidateVisibleBounds);
}

RenderedPointCloudData::~RenderedPointCloudData() = default;

PointCloudDataObject & RenderedPointCloudData::pointCloudDataObject()
{
    return static_cast<PointCloudDataObject &>(dataObject());
}

const PointCloudDataObject & RenderedPointCloudData::pointCloudDataObject() const
{
    return static_cast<const PointCloudDataObject &>(dataObject());
}

std::unique_ptr<PropertyGroup> RenderedPointCloudData::createConfigGroup()
{
    auto renderSettings = RenderedData3D::createConfigGroup();

    renderSettings->addProperty<Color>("Color",
        [this] () {
        double * color = renderProperty()->GetColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [this] (const Color & color) {
        renderProperty()->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    });

    renderSettings->addProperty<unsigned>("pointSize",
        [this] () {
        return static_cast<unsigned>(renderProperty()->GetPointSize());
    },
        [this] (unsigned pointSize) {
        renderProperty()->SetPointSize(static_cast<float>(pointSize));
        emit geometryChanged();
    })
        ->setOptions({
            { "title", "Point Size" },
            { "minimum", 1 },
            { "maximum", 20 },
            { "step", 1 },
            { "suffix", " pixel" }
    });

    renderSettings->addProperty<double>("Transparency",
        [this]() {
        return (1.0 - renderProperty()->GetOpacity()) * 100;
    },
        [this](double transparency) {
        renderProperty()->SetOpacity(1.0 - transparency * 0.01);
        emit geometryChanged();
    })
        ->setOptions({
            { "minimum", 0 },
            { "maximum", 100 },
            { "step", 1 },
            { "suffix", " %" }
    });

    return renderSettings;
}

vtkSmartPointer<vtkProperty> RenderedPointCloudData::createDefaultRenderProperty() const
{
    auto prop = vtkSmartPointer<vtkProperty>::New();
    prop->SetRepresentationToPoints();
    prop->SetPointSize(3.0);
    prop->SetColor(0, 0.6, 0);
    prop->SetOpacity(1.0);
    prop->SetInterpolationToFlat();
    prop->EdgeVisibilityOff();
    prop->BackfaceCullingOff();
    prop->LightingOff();

    return prop;
}

vtkSmartPointer<vtkProp3DCollection> RenderedPointCloudData::fetchViewProps3D()
{
    auto actors = RenderedData3D::fetchViewProps3D();
    actors->AddItem(mainActor());

    return actors;
}

vtkActor * RenderedPointCloudData::mainActor()
{
    if (m_mainActor)
    {
        return m_mainActor;
    }

    finalizePipeline();

    m_mainActor = vtkSmartPointer<vtkActor>::New();
    m_mainActor->SetMapper(m_mapper);
    m_mainActor->SetProperty(renderProperty());

    return m_mainActor;
}

void RenderedPointCloudData::scalarsForColorMappingChangedEvent()
{
    RenderedData3D::scalarsForColorMappingChangedEvent();

    // no mapping yet, so just render the data set
    if (!currentColorMappingData())
    {
        m_colorMappingOutput = colorMappingInput()->GetProducer();
        finalizePipeline();
        return;
    }

    currentColorMappingData()->configureMapper(*this, *m_mapper);

    vtkSmartPointer<vtkAlgorithm> filter;

    if (currentColorMappingData()->usesFilter())
    {
        filter = currentColorMappingData()->createFilter(*this);
        m_colorMappingOutput = filter;
    }
    else
    {
        m_colorMappingOutput = colorMappingInput()->GetProducer();
    }

    finalizePipeline();
}

void RenderedPointCloudData::colorMappingGradientChangedEvent()
{
    RenderedData3D::colorMappingGradientChangedEvent();

    m_mapper->SetLookupTable(currentColorMappingGradient());
}

void RenderedPointCloudData::visibilityChangedEvent(bool visible)
{
    RenderedData3D::visibilityChangedEvent(visible);

    mainActor()->SetVisibility(visible);
}

DataBounds RenderedPointCloudData::updateVisibleBounds()
{
    DataBounds bounds;
    mainActor()->GetBounds(bounds.data());
    return bounds;
}

void RenderedPointCloudData::finalizePipeline()
{
    // no color mapping set up yet
    if (!m_colorMappingOutput)
    {
        // disabled color mapping and texturing by default
        m_colorMappingOutput = colorMappingInput()->GetProducer();
        m_mapper->ScalarVisibilityOff();
    }

    m_mapper->SetInputConnection(m_colorMappingOutput->GetOutputPort());
}
