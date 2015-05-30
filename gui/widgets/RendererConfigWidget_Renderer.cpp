#include "RendererConfigWidget.h"
#include "ui_RendererConfigWidget.h"

#include <cassert>
#include <limits>

#include <vtkRenderer.h>
#include <vtkLightKit.h>
#include <vtkCamera.h>
#include <vtkCubeAxesActor.h>
#include <vtkMath.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkTextWidget.h>

#include <reflectionzeug/PropertyGroup.h>
#include <core/types.h>
#include <core/utility/vtkcamerahelper.h>
#include <core/reflectionzeug_extension/QStringProperty.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>


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
    assert(m_currentRenderView && dynamic_cast<RendererImplementationBase3D *>(&m_currentRenderView->implementation()));
    assert(static_cast<RendererImplementationBase3D *>(&m_currentRenderView->implementation())->camera() == camera);

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
        RendererImplementationBase3D & impl3D = static_cast<RendererImplementationBase3D &>(m_currentRenderView->implementation());
        double azimuth = TerrainCamera::getAzimuth(*camera);
        if (TerrainCamera::getVerticalElevation(*camera) < 0)
            azimuth *= -1;

        impl3D.axesActor()->GetLabelTextProperty(0)->SetOrientation(azimuth - 90);
        impl3D.axesActor()->GetLabelTextProperty(1)->SetOrientation(azimuth);
    }
}

reflectionzeug::PropertyGroup * RendererConfigWidget::createPropertyGroupRenderer(AbstractRenderView * renderView, RendererImplementationBase3D * impl)
{
    PropertyGroup * root = new PropertyGroup();

    if (renderView->numberOfSubViews() == 1)
    {
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
    }
    else
    {
        auto titlesGroup = root->addGroup("Titles");

        for (unsigned i = 0; i < renderView->numberOfSubViews(); ++i)
        {
            titlesGroup->addProperty<QString>("Title" + std::to_string(i),
                [impl, i] () { return QString::fromUtf8(impl->titleWidget(i)->GetTextActor()->GetInput()); },
                [impl, renderView, i] (const QString & title) {
                impl->titleWidget(i)->GetTextActor()->SetInput(title.toUtf8().data());
                renderView->render();
            })
                ->setOption("title", "Title " + std::to_string(i + 1));

            titlesGroup->addProperty<int>("TitleFontSize" + std::to_string(i),
                [impl, i] () { return impl->titleWidget(i)->GetTextActor()->GetTextProperty()->GetFontSize(); },
                [impl, renderView, i] (const int & fontSize) {
                impl->titleWidget(i)->GetTextActor()->GetTextProperty()->SetFontSize(fontSize);
                renderView->render();
            })
                ->setOption("title", "Font Size");
        }
    }

    auto firstRenderer = impl->renderer();

    auto backgroundColor = root->addProperty<Color>("backgroundColor",
        [firstRenderer] () {
        double * color = firstRenderer->GetBackground();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [renderView, impl] (const Color & color) {
        for (unsigned i = 0; i < renderView->numberOfSubViews(); ++i)
            impl->renderer(i)->SetBackground(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
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
                [&camera, impl] (const double & azimuth) {
                TerrainCamera::setAzimuth(camera, azimuth);
                impl->renderer()->ResetCameraClippingRange();
                impl->render();
            });
            prop_azimuth->setOption("minimum", std::numeric_limits<double>::lowest());
            prop_azimuth->setOption("suffix", degreesSuffix);

            auto prop_elevation = cameraGroup->addProperty<double>("Elevation",
                [&camera]() {
                return TerrainCamera::getVerticalElevation(camera);
            },
                [&camera, impl] (const double & elevation) {
                TerrainCamera::setVerticalElevation(camera, elevation);
                impl->renderer()->ResetCameraClippingRange();
                impl->render();
            });
            prop_elevation->setOption("minimum", -87);
            prop_elevation->setOption("maximum", 87);
            prop_elevation->setOption("suffix", degreesSuffix);
        }

        auto prop_focalPoint = cameraGroup->addProperty<std::array<double, 3>>("focalPoint",
            [&camera](size_t component) {
            return camera.GetFocalPoint()[component];
        },
            [&camera, impl](size_t component, const double & value) {
            double foc[3];
            camera.GetFocalPoint(foc);
            foc[component] = value;
            camera.SetFocalPoint(foc);
            impl->renderer()->ResetCameraClippingRange();
            impl->render();
        });
        prop_focalPoint->setOption("title", "Focal Point");
        char title[2] {'X', 0};
        for (quint8 i = 0; i < 3; ++i)
        {
            prop_focalPoint->at(i)->setOptions({
                { "title", std::string(title) },
                { "minimum", std::numeric_limits<float>::lowest() } });
            ++title[0];
        }

        auto prop_distance = cameraGroup->addProperty<double>("distance",
            [&camera] (){ return camera.GetDistance(); },
            [&camera, impl] (double d) {
            double viewVec[3], focalPoint[3], position[3];
            camera.GetDirectionOfProjection(viewVec);
            camera.GetFocalPoint(focalPoint);
            
            vtkMath::MultiplyScalar(viewVec, d);
            vtkMath::Subtract(focalPoint, viewVec, position);
            camera.SetPosition(position);

            impl->renderer()->ResetCameraClippingRange();
            impl->render();
        });
        prop_distance->setOptions({
            { "title", "Distance" },
            { "minimum", 0.001f }
        });

        if (contains3dData)
        {
            auto prop_projectionType = cameraGroup->addProperty<ProjectionType>("Projection",
                [&camera] () {
                return camera.GetParallelProjection() != 0 ? ProjectionType::parallel : ProjectionType::perspective;
            },
                [&camera, impl] (ProjectionType type) {
                camera.SetParallelProjection(type == ProjectionType::parallel);
                impl->renderer()->ResetCameraClippingRange();
                impl->render();
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
        axisTitlesGroup->addProperty<QString>("X",
            [impl] () { return QString::fromUtf8(impl->axesActor()->GetXTitle()); },
            [impl, renderView] (const QString & label) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
                impl->axesActor(i)->SetXTitle(label.toUtf8().data());
            renderView->render();
        });
        axisTitlesGroup->addProperty<QString>("Y",
            [impl] () { return QString::fromUtf8(impl->axesActor()->GetYTitle()); },
            [impl, renderView] (const QString & label) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
                impl->axesActor(i)->SetYTitle(label.toUtf8().data());
            renderView->render();
        });
        axisTitlesGroup->addProperty<QString>("Z",
            [impl] () { return QString::fromUtf8(impl->axesActor()->GetZTitle()); },
            [impl, renderView] (const QString & label) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
                impl->axesActor(i)->SetZTitle(label.toUtf8().data());
            renderView->render();
        });
    }

    auto axesGroup = root->addGroup("Axes");
    {
        axesGroup->addProperty<bool>("Visible",
            std::bind(&AbstractRenderView::axesEnabled, renderView),
            std::bind(&AbstractRenderView::setEnableAxes, renderView, std::placeholders::_1));

        axesGroup->addProperty<bool>("Show Labels",
            [impl] () { return impl->axesActor()->GetXAxisLabelVisibility() != 0; },
            [impl, renderView] (bool visible) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
            {
                impl->axesActor(i)->SetXAxisLabelVisibility(visible);
                impl->axesActor(i)->SetYAxisLabelVisibility(visible);
                impl->axesActor(i)->SetZAxisLabelVisibility(visible);
            }
            renderView->render();
        });

        auto prop_gridVisible = axesGroup->addProperty<bool>("gridLines",
            [impl] () { return impl->axesActor()->GetDrawXGridlines() != 0; },
            [impl, renderView] (bool visible) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
            {
                impl->axesActor(i)->SetDrawXGridlines(visible);
                impl->axesActor(i)->SetDrawYGridlines(visible);
                impl->axesActor(i)->SetDrawZGridlines(visible);
            }
            renderView->render();
        });
        prop_gridVisible->setOption("title", "Grid Lines");

        auto prop_innerGridVisible = axesGroup->addProperty<bool>("innerGridLines",
            [impl] () { return impl->axesActor()->GetDrawXInnerGridlines() != 0; },
            [impl, renderView] (bool visible) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
            {
                impl->axesActor(i)->SetDrawXInnerGridlines(visible);
                impl->axesActor(i)->SetDrawYInnerGridlines(visible);
                impl->axesActor(i)->SetDrawZInnerGridlines(visible);
            }
            renderView->render();
        });
        prop_innerGridVisible->setOption("title", "Inner Grid Lines");

        auto prop_foregroundGridLines = axesGroup->addProperty<bool>("foregroundGridLines",
            [impl] () { return impl->axesActor()->GetGridLineLocation() == VTK_GRID_LINES_ALL; },
            [impl, renderView] (bool v) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
            {
                impl->axesActor(i)->SetGridLineLocation(v
                    ? VTK_GRID_LINES_ALL
                    : VTK_GRID_LINES_FURTHEST);
            }
            renderView->render();
        });
        prop_foregroundGridLines->setOption("title", "Foreground Grid Lines");

        axesGroup->addProperty<bool>("Ticks",
            [impl] () { return impl->axesActor()->GetXAxisTickVisibility() != 0; },
            [impl, renderView] (bool visible) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
            {
                impl->axesActor(i)->SetXAxisTickVisibility(visible);
                impl->axesActor(i)->SetYAxisTickVisibility(visible);
                impl->axesActor(i)->SetZAxisTickVisibility(visible);
            }
            renderView->render();
        });

        auto prop_minorTicksVisible = axesGroup->addProperty<bool>("minorTicks",
            [impl] () { return impl->axesActor()->GetXAxisMinorTickVisibility() != 0; },
            [impl, renderView] (bool visible) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
            {
                impl->axesActor(i)->SetXAxisMinorTickVisibility(visible);
                impl->axesActor(i)->SetYAxisMinorTickVisibility(visible);
                impl->axesActor(i)->SetZAxisMinorTickVisibility(visible);
            }
            renderView->render();
        });
        prop_minorTicksVisible->setOption("title", "Minor Ticks");

        auto prop_tickLocation = axesGroup->addProperty<CubeAxesTickLocation>("tickLocation",
            [impl] () { return static_cast<CubeAxesTickLocation>(impl->axesActor()->GetTickLocation()); },
            [impl, renderView] (CubeAxesTickLocation mode) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
                impl->axesActor(i)->SetTickLocation(static_cast<int>(mode));
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
