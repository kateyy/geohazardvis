#include "ImageProfilePlot.h"

#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkProp3DCollection.h>
#include <vtkPolyDataMapper.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/data_objects/ImageProfileData.h>

using namespace reflectionzeug;


ImageProfilePlot::ImageProfilePlot(ImageProfileData * dataObject)
    : RenderedData3D(dataObject)
{
}

PropertyGroup * ImageProfilePlot::createConfigGroup()
{
    PropertyGroup * renderSettings = new PropertyGroup();

    renderSettings->addProperty<Color>("color",
        [this] () {
        double * color = renderProperty()->GetColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [this] (const Color & color) {
        renderProperty()->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    });

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

    auto transparency = renderSettings->addProperty<double>("transparency",
        [this] () {
        return (1.0 - renderProperty()->GetOpacity()) * 100;
    },
        [this] (double transparency) {
        renderProperty()->SetOpacity(1.0 - transparency * 0.01);
        emit geometryChanged();
    });
    transparency->setOption("minimum", 0);
    transparency->setOption("maximum", 100);
    transparency->setOption("step", 1);
    transparency->setOption("suffix", " %");

    return renderSettings;
}

vtkProperty * ImageProfilePlot::createDefaultRenderProperty() const
{
    vtkProperty * property = vtkProperty::New();
    property->LightingOff();
    property->SetColor(1, 0, 0);
    property->SetLineWidth(2);
    return property;
}

vtkSmartPointer<vtkProp3DCollection> ImageProfilePlot::fetchViewProps3D()
{
    auto props = RenderedData3D::fetchViewProps3D();
    props->AddItem(plotActor());

    return props;
}

vtkActor * ImageProfilePlot::plotActor()
{
    if (!m_plotActor)
    {
        VTK_CREATE(vtkPolyDataMapper, mapper);
        mapper->SetInputConnection(dataObject()->processedOutputPort());
        mapper->ScalarVisibilityOff();

        m_plotActor = vtkSmartPointer<vtkActor>::New();
        m_plotActor->SetMapper(mapper);
        m_plotActor->SetProperty(renderProperty());
    }

    return m_plotActor;
}

void ImageProfilePlot::visibilityChangedEvent(bool visible)
{
    RenderedData3D::visibilityChangedEvent(visible);

    plotActor()->SetVisibility(visible);
}
