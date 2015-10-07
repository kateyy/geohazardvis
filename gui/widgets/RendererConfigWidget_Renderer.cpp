#include "RendererConfigWidget.h"
#include "ui_RendererConfigWidget.h"

#include <cassert>
#include <limits>

#include <vtkRenderer.h>
#include <vtkLightKit.h>
#include <vtkCamera.h>
#include <vtkMath.h>
#include <vtkProperty.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkTextWidget.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/types.h>
#include <core/utility/macros.h>
#include <core/utility/vtkcamerahelper.h>
#include <core/reflectionzeug_extension/QStringProperty.h>
#include <core/ThirdParty/ParaView/vtkGridAxes3DActor.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>


using namespace reflectionzeug;
using namespace propertyguizeug;

namespace
{
enum class ProjectionType
{
    parallel,
    perspective
};
}

void RendererConfigWidget::readCameraStats(vtkObject * DEBUG_ONLY(caller), unsigned long, void *)
{
    assert(m_currentRenderView && dynamic_cast<RendererImplementationBase3D *>(&m_currentRenderView->implementation()));
    // assuming synchronized cameras for now
    assert(vtkCamera::SafeDownCast(caller));
    assert(static_cast<RendererImplementationBase3D *>(&m_currentRenderView->implementation())->camera(0) == caller);

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
}

reflectionzeug::PropertyGroup * RendererConfigWidget::createPropertyGroupRenderer(AbstractRenderView * renderView, RendererImplementationBase3D * impl)
{
    PropertyGroup * root = new PropertyGroup();

    if (renderView->numberOfSubViews() == 1)
    {
        root->addProperty<QString>("Title",
            [impl] () { return QString::fromUtf8(impl->titleWidget(0)->GetTextActor()->GetInput()); },
            [impl, renderView] (const QString & title) {
            impl->titleWidget(0)->GetTextActor()->SetInput(title.toUtf8().data());
            renderView->render();
        });

        root->addProperty<int>("TitleFontSize",
            [impl] () { return impl->titleWidget(0)->GetTextActor()->GetTextProperty()->GetFontSize(); },
            [impl, renderView] (const int & fontSize) {
            impl->titleWidget(0)->GetTextActor()->GetTextProperty()->SetFontSize(fontSize);
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

    auto firstRenderer = impl->renderer(0);

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

    bool is3DView = renderView->implementation().currentInteractionStrategy() == "3D terrain";

    auto cameraGroup = root->addGroup("Camera");
    {
        // assuming synchronized cameras for now
        vtkCamera & camera = *impl->camera(0);

        if (is3DView)
        {
            std::string degreesSuffix = (QString(' ') + QChar(0xB0)).toStdString();

            auto prop_azimuth = cameraGroup->addProperty<double>("Azimuth",
                [&camera]() {
                return TerrainCamera::getAzimuth(camera);
            },
                [&camera, impl] (const double & azimuth) {
                TerrainCamera::setAzimuth(camera, azimuth);
                impl->renderer(0)->ResetCameraClippingRange();
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
                impl->renderer(0)->ResetCameraClippingRange();
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
            impl->renderer(0)->ResetCameraClippingRange();
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

            impl->renderer(0)->ResetCameraClippingRange();
            impl->render();
        });
        prop_distance->setOptions({
            { "title", "Distance" },
            { "minimum", 0.001f }
        });

        if (is3DView)
        {
            auto prop_projectionType = cameraGroup->addProperty<ProjectionType>("Projection",
                [&camera] () {
                return camera.GetParallelProjection() != 0 ? ProjectionType::parallel : ProjectionType::perspective;
            },
                [&camera, impl] (ProjectionType type) {
                camera.SetParallelProjection(type == ProjectionType::parallel);
                impl->renderer(0)->ResetCameraClippingRange();
                impl->render();
            });
            prop_projectionType->setStrings({
                    { ProjectionType::parallel, "parallel" },
                    { ProjectionType::perspective, "perspective" }
            });
        }
    }

    if (is3DView)
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
        for (char axis = 0; axis < 3; ++axis)
        {
            auto title = std::string{ char('X' + axis) };

            axisTitlesGroup->addProperty<std::string>(title,
                [impl, axis] () { return impl->axesActor(0)->GetTitle(axis); },
                [impl, renderView, axis] (const std::string & label) {
                for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
                    impl->axesActor(i)->SetTitle(axis, label);
                renderView->render();
            });
        }
    }

    auto axesGroup = root->addGroup("Axes");
    {
        axesGroup->addProperty<bool>("Visible",
            std::bind(&AbstractRenderView::axesEnabled, renderView),
            std::bind(&AbstractRenderView::setEnableAxes, renderView, std::placeholders::_1));

        axesGroup->addProperty<Color>("Color",
            [impl] () {
            const double * color = impl->axesActor(0)->GetProperty()->GetColor();
            return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
        },
            [renderView, impl] (const Color & color) {
            for (unsigned i = 0; i < renderView->numberOfSubViews(); ++i)
                impl->axesActor(i)->GetProperty()->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
            renderView->render();
        });

        axesGroup->addProperty<bool>("ShowLabels",
            [impl] () { return impl->axesActor(0)->GetLabelMask() != 0; },
            [impl, renderView] (bool visible) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
            {
                impl->axesActor(i)->SetLabelMask(visible ? 0xFF : 0x00);
            }
            renderView->render();
        })
            ->setOption("title", "Show Labels");

        axesGroup->addProperty<bool>("gridLines",
            [impl] () { return impl->axesActor(0)->GetGenerateGrid(); },
            [impl, renderView] (bool visible) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
            {
                impl->axesActor(i)->SetGenerateGrid(visible);
            }
            renderView->render();
        })
            ->setOption("title", "Grid Lines");

        axesGroup->addProperty<bool>("foregroundGridLines",
            [impl] () { return impl->axesActor(0)->GetProperty()->GetFrontfaceCulling() == 0; },
            [impl, renderView] (bool v) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
            {
                impl->axesActor(i)->GetProperty()->SetFrontfaceCulling(!v);
                // render unique labels only if we skip foreground axes
                // otherwise, we won't have any labels when rendering all axes
                impl->axesActor(i)->SetLabelUniqueEdgesOnly(!v);
            }
            renderView->render();
        })
            ->setOption("title", "Foreground Grid Lines");

        axesGroup->addProperty<bool>("Ticks",
            [impl] () { return impl->axesActor(0)->GetGenerateTicks(); },
            [impl, renderView] (bool visible) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
            {
                impl->axesActor(i)->SetGenerateTicks(visible);
            }
            renderView->render();
        });

        axesGroup->addProperty<bool>("Edges",
            [impl] () { return impl->axesActor(0)->GetGenerateEdges(); },
            [impl, renderView] (bool visible) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
            {
                impl->axesActor(i)->SetGenerateEdges(visible);
            }
            renderView->render();
        });
    }

    return root;
}
