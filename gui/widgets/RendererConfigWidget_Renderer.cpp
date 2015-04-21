#include "RendererConfigWidget.h"
#include "ui_RendererConfigWidget.h"

#include <cassert>
#include <limits>

#include <vtkRenderer.h>
#include <vtkLightKit.h>
#include <vtkCamera.h>
#include <vtkCubeAxesActor.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkTextWidget.h>

#include <reflectionzeug/PropertyGroup.h>
#include <core/types.h>
#include <core/vtkcamerahelper.h>
#include <core/reflectionzeug_extension/QStringProperty.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementation3D.h>


using namespace reflectionzeug;
using namespace propertyguizeug;

namespace
{
enum class CubeAxesTickLocation
{
    inside = VTK_TICKS_INSIDE,
    outside = VTK_TICKS_OUTSIDE,
    both = VTK_TICKS_BOTH
};
enum class ProjectionType
{
    parallel,
    perspective
};
}

void RendererConfigWidget::readCameraStats(vtkObject * caller)
{
    assert(vtkCamera::SafeDownCast(caller));
    vtkCamera * camera = static_cast<vtkCamera *>(caller);
    assert(m_currentRenderView && dynamic_cast<RendererImplementation3D *>(
        &m_currentRenderView->selectedViewImplementation()));
    assert(static_cast<RendererImplementation3D *>(&m_currentRenderView->selectedViewImplementation())->camera() == camera);

    std::function<void(AbstractProperty &)> updateFunc = [&updateFunc] (AbstractProperty & property)
    {
        if (property.isCollection())
            property.asCollection()->forEach(updateFunc);
        if (property.isValue())
            property.asValue()->valueChanged();
    };

    PropertyGroup * cameraGroup = nullptr;
    if (m_propertyRoot->propertyExists("Camera"))
        cameraGroup = m_propertyRoot->group("Camera");

    if (cameraGroup)
        cameraGroup->forEach(updateFunc);

    // update axes text label rotations for terrain view
    if (m_currentRenderView->contentType() == ContentType::Rendered3D)
    {
        RendererImplementation3D & impl3D = static_cast<RendererImplementation3D &>(m_currentRenderView->selectedViewImplementation());
        double azimuth = TerrainCamera::getAzimuth(*camera);
        if (TerrainCamera::getVerticalElevation(*camera) < 0)
            azimuth *= -1;

        impl3D.axesActor()->GetLabelTextProperty(0)->SetOrientation(azimuth - 90);
        impl3D.axesActor()->GetLabelTextProperty(1)->SetOrientation(azimuth);
    }
}

reflectionzeug::PropertyGroup * RendererConfigWidget::createPropertyGroupRenderer(AbstractRenderView * renderView, RendererImplementation3D * impl)
{
    PropertyGroup * root = new PropertyGroup();
    vtkRenderer * renderer = impl->renderer();

    root->addProperty<QString>("Title",
        [impl] () { return QString::fromUtf8(impl->titleWidget()->GetTextActor()->GetInput()); },
        [impl, renderView] (const QString & title) {
        impl->titleWidget()->GetTextActor()->SetInput(title.toUtf8().data());
        renderView->render();
    });

    root->addProperty<int>("TitleFontSize",
        [impl] () { return impl->titleWidget()->GetTextActor()->GetTextProperty()->GetFontSize(); },
        [impl, renderView] (const int & fontSize) {
        impl->titleWidget()->GetTextActor()->GetTextProperty()->SetFontSize(fontSize);
        renderView->render();
    })
        ->setOption("title", "Title Font Size");

    auto backgroundColor = root->addProperty<Color>("backgroundColor",
        [renderer]() {
        double * color = renderer->GetBackground();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [renderView, renderer] (const Color & color) {
        renderer->SetBackground(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        renderView->render();
    });
    backgroundColor->setOption("title", "Background Color");

    bool contains3dData = renderView->contentType() == ContentType::Rendered3D;

    auto cameraGroup = root->addGroup("Camera");
    {
        vtkCamera & camera = *impl->camera();

        if (contains3dData)
        {
            std::string degreesSuffix = (QString(' ') + QChar(0xB0)).toStdString();

            auto prop_azimuth = cameraGroup->addProperty<double>("Azimuth",
                [&camera]() {
                return TerrainCamera::getAzimuth(camera);
            },
                [renderView, &camera, renderer] (const double & azimuth) {
                TerrainCamera::setAzimuth(camera, azimuth);
                renderer->ResetCamera();
                renderView->render();
            });
            prop_azimuth->setOption("minimum", std::numeric_limits<double>::lowest());
            prop_azimuth->setOption("suffix", degreesSuffix);

            auto prop_elevation = cameraGroup->addProperty<double>("Elevation",
                [&camera]() {
                return TerrainCamera::getVerticalElevation(camera);
            },
                [renderView, &camera, renderer] (const double & elevation) {
                TerrainCamera::setVerticalElevation(camera, elevation);
                renderer->ResetCamera();
                renderView->render();
            });
            prop_elevation->setOption("minimum", -89);
            prop_elevation->setOption("maximum", 89);
            prop_elevation->setOption("suffix", degreesSuffix);
        }

        auto prop_focalPoint = cameraGroup->addProperty<std::array<double, 3>>("focalPoint",
            [&camera](size_t component) {
            return camera.GetFocalPoint()[component];
        },
            [&camera, renderView](size_t component, const double & value) {
            double foc[3];
            camera.GetFocalPoint(foc);
            foc[component] = value;
            camera.SetFocalPoint(foc);
            renderView->render();
        });
        prop_focalPoint->setOption("title", "Focal Point");
        char title[2] {'x', 0};
        for (quint8 i = 0; i < 3; ++i)
        {
            prop_focalPoint->at(i)->setOption("title", std::string(title));
            ++title[0];
        }


        if (contains3dData)
        {
            auto prop_projectionType = cameraGroup->addProperty<ProjectionType>("Projection",
                [&camera] () {
                return camera.GetParallelProjection() != 0 ? ProjectionType::parallel : ProjectionType::perspective;
            },
                [&camera, renderView] (ProjectionType type) {
                camera.SetParallelProjection(type == ProjectionType::parallel);
                renderView->render();
            });
            prop_projectionType->setStrings({
                    { ProjectionType::parallel, "parallel" },
                    { ProjectionType::perspective, "perspective" }
            });
        }
    }

    if (contains3dData)
    {
        PropertyGroup * lightingGroup = root->addGroup("Lighting");

        auto prop_intensity = lightingGroup->addProperty<double>("Intensity",
            [renderView, impl]() { return impl->lightKit()->GetKeyLightIntensity(); },
            [renderView, impl] (double value) {
            impl->lightKit()->SetKeyLightIntensity(value);
            renderView->render();
        });
        prop_intensity->setOption("minimum", 0);
        prop_intensity->setOption("maximum", 3);
        prop_intensity->setOption("step", 0.05);
    }

    auto axisTitlesGroup = root->addGroup("AxisTitles");
    axisTitlesGroup->setOption("title", "Axis Titles");
    {
        vtkCubeAxesActor * axes = impl->axesActor();

        axisTitlesGroup->addProperty<QString>("X",
            [axes] () { return QString::fromUtf8(axes->GetXTitle()); },
            [renderView, axes] (const QString & label) {
            axes->SetXTitle(label.toUtf8().data());
            renderView->render();
        });
        axisTitlesGroup->addProperty<QString>("Y",
            [axes] () { return QString::fromUtf8(axes->GetYTitle()); },
            [renderView, axes] (const QString & label) {
            axes->SetYTitle(label.toUtf8().data());
            renderView->render();
        });
        axisTitlesGroup->addProperty<QString>("Z",
            [axes] () { return QString::fromUtf8(axes->GetZTitle()); },
            [renderView, axes] (const QString & label) {
            axes->SetZTitle(label.toUtf8().data());
            renderView->render();
        });
    }

    auto axesGroup = root->addGroup("Axes");
    {
        vtkCubeAxesActor * axes = impl->axesActor();
        axesGroup->addProperty<bool>("Visible",
            std::bind(&AbstractRenderView::axesEnabled, renderView),
            std::bind(&AbstractRenderView::setEnableAxes, renderView, std::placeholders::_1));

        axesGroup->addProperty<bool>("Show Labels",
            [axes] () { return axes->GetXAxisLabelVisibility() != 0; },
            [axes, renderView] (bool visible) {
            axes->SetXAxisLabelVisibility(visible);
            axes->SetYAxisLabelVisibility(visible);
            axes->SetZAxisLabelVisibility(visible);
            renderView->render();
        });

        auto prop_gridVisible = axesGroup->addProperty<bool>("gridLines",
            [axes] () { return axes->GetDrawXGridlines() != 0; },
            [axes, renderView] (bool visible) {
            axes->SetDrawXGridlines(visible);
            axes->SetDrawYGridlines(visible);
            axes->SetDrawZGridlines(visible);
            renderView->render();
        });
        prop_gridVisible->setOption("title", "Grid Lines");

        auto prop_innerGridVisible = axesGroup->addProperty<bool>("innerGridLines",
            [axes] () { return axes->GetDrawXInnerGridlines() != 0; },
            [axes, renderView] (bool visible) {
            axes->SetDrawXInnerGridlines(visible);
            axes->SetDrawYInnerGridlines(visible);
            axes->SetDrawZInnerGridlines(visible);
            renderView->render();
        });
        prop_innerGridVisible->setOption("title", "Inner Grid Lines");

        auto prop_foregroundGridLines = axesGroup->addProperty<bool>("foregroundGridLines",
            [axes] () { return axes->GetGridLineLocation() == VTK_GRID_LINES_ALL; },
            [axes, renderView] (bool v) {
            axes->SetGridLineLocation(v
                ? VTK_GRID_LINES_ALL
                : VTK_GRID_LINES_FURTHEST);
            renderView->render();
        });
        prop_foregroundGridLines->setOption("title", "Foreground Grid Lines");

        axesGroup->addProperty<bool>("Ticks",
            [axes] () { return axes->GetXAxisTickVisibility() != 0; },
            [axes, renderView] (bool visible) {
            axes->SetXAxisTickVisibility(visible);
            axes->SetYAxisTickVisibility(visible);
            axes->SetZAxisTickVisibility(visible);
            renderView->render();
        });

        auto prop_minorTicksVisible = axesGroup->addProperty<bool>("minorTicks",
            [axes] () { return axes->GetXAxisMinorTickVisibility() != 0; },
            [axes, renderView] (bool visible) {
            axes->SetXAxisMinorTickVisibility(visible);
            axes->SetYAxisMinorTickVisibility(visible);
            axes->SetZAxisMinorTickVisibility(visible);
            renderView->render();
        });
        prop_minorTicksVisible->setOption("title", "Minor Ticks");

        auto prop_tickLocation = axesGroup->addProperty<CubeAxesTickLocation>("tickLocation",
            [axes] () { return static_cast<CubeAxesTickLocation>(axes->GetTickLocation()); },
            [axes, renderView] (CubeAxesTickLocation mode) {
            axes->SetTickLocation(static_cast<int>(mode));
            renderView->render();
        });
        prop_tickLocation->setOption("title", "Tick Location");
        prop_tickLocation->setStrings({
                { CubeAxesTickLocation::both, "inside/outside" },
                { CubeAxesTickLocation::inside, "inside" },
                { CubeAxesTickLocation::outside, "outside" } }
        );
    }

    return root;
}
