#include "RendererConfigWidget.h"

#include <cassert>

#include <vtkLightKit.h>
#include <vtkProperty.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/ThirdParty/ParaView/vtkGridAxes3DActor.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>


using namespace reflectionzeug;

void RendererConfigWidget::createPropertyGroupRenderer2(PropertyGroup * root, AbstractRenderView * renderView, RendererImplementationBase3D * impl)
{
    bool is3DView = renderView->implementation().currentInteractionStrategy() == "3D terrain";

    if (is3DView)
    {
        PropertyGroup * lightingGroup = root->addGroup("Lighting");

        lightingGroup->addProperty<double>("Intensity",
            [renderView, impl]() { return impl->lightKit()->GetKeyLightIntensity(); },
            [renderView, impl] (double value) {
            impl->lightKit()->SetKeyLightIntensity(value);
            renderView->render();
        })
            ->setOptions({
                { "minimum", 0 },
                { "maximum", 3 },
                { "step", 0.05 },
        });
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
}