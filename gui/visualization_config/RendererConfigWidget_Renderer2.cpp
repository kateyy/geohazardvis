#include "RendererConfigWidget.h"

#include <cassert>

#include <vtkLightKit.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/config.h>
#include <core/ThirdParty/ParaView/vtkGridAxes3DActor.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>

#if VTK_RENDERING_BACKEND == 2
#include <vtkFXAAOptions.h>
#endif


using namespace reflectionzeug;

void RendererConfigWidget::createPropertyGroupRenderer2(PropertyGroup & root, AbstractRenderView * renderView, RendererImplementationBase3D * impl)
{
    bool is3DView = renderView->implementation().currentInteractionStrategy() == "3D terrain";

    if (is3DView)
    {
        PropertyGroup * lightingGroup = root.addGroup("Lighting");

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

    auto axisTitlesGroup = root.addGroup("AxisTitles");
    axisTitlesGroup->setOption("title", "Axis Titles");
    {
        for (char axis = 0; axis < 3; ++axis)
        {
            auto title = std::string{ char('X' + axis) };

            axisTitlesGroup->addProperty<std::string>(title,
                [impl, axis] () { return impl->axesActor(0)->GetTitle(axis); },
                [impl, renderView, axis] (const std::string & label) {
                for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
                {
                    impl->axesActor(i)->SetTitle(axis, label);
                }
                renderView->render();
            });
        }
    }

    auto axesGroup = root.addGroup("Axes");
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
            {
                impl->axesActor(i)->GetProperty()->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
            }
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

#if VTK_RENDERING_BACKEND == 2
    root.addProperty<bool>("FXAA",
        [impl] () { return impl->renderer(0)->GetUseFXAA(); },
        [impl] (bool enable)
    {
        for (unsigned i = 0; i < impl->renderView().numberOfSubViews(); ++i)
        {
            impl->renderer(i)->SetUseFXAA(enable);
        }
        impl->renderView().render();
    })
        ->setOptions({
            { "title", "Enable FXAA" },
            { "tooltip", "Anti-Aliasing smooths out the jagged edges of lines and polygons\n"
            "The performance of FXAA (Fast approXimate Anti-Aliasing) depends only on the size of "
            "the render window."}
    });

    auto fxaaOptionsGroup = root.addGroup("FXAAOptions");
    fxaaOptionsGroup->setOption("title", "FXAA Options");

    auto fxaaOptions = vtkSmartPointer<vtkFXAAOptions>::New();

    fxaaOptionsGroup->addProperty<float>("RelativeContrastThreshold",
        [impl] () { return impl->renderer(0)->GetFXAAOptions()->GetRelativeContrastThreshold(); },
        [impl] (float threshold)
    {
        for (unsigned i = 0; i < impl->renderView().numberOfSubViews(); ++i)
        {
            impl->renderer(i)->GetFXAAOptions()->SetRelativeContrastThreshold(threshold);
        }
        impl->renderView().render();
    })
        ->setOptions({
            { "title", "Relative Contrast Threshold" },
            { "min", fxaaOptions->GetRelativeContrastThresholdMinValue() },
            { "max", fxaaOptions->GetRelativeContrastThresholdMaxValue() },
            { "step", 0.01f }
    });

    fxaaOptionsGroup->addProperty<float>("HardContrastThreshold",
        [impl] () { return impl->renderer(0)->GetFXAAOptions()->GetHardContrastThreshold(); },
        [impl] (float threshold)
    {
        for (unsigned i = 0; i < impl->renderView().numberOfSubViews(); ++i)
        {
            impl->renderer(i)->GetFXAAOptions()->SetHardContrastThreshold(threshold);
        }
        impl->renderView().render();
    })
        ->setOptions({
            { "title", "Hard Contrast Threshold" },
            { "min", fxaaOptions->GetHardContrastThresholdMinValue() },
            { "max", fxaaOptions->GetHardContrastThresholdMaxValue() },
            { "step", 0.01f }
    });

    fxaaOptionsGroup->addProperty<float>("SubpixelBlendLimit",
        [impl] () { return impl->renderer(0)->GetFXAAOptions()->GetSubpixelBlendLimit(); },
        [impl] (float threshold)
    {
        for (unsigned i = 0; i < impl->renderView().numberOfSubViews(); ++i)
        {
            impl->renderer(i)->GetFXAAOptions()->SetSubpixelBlendLimit(threshold);
        }
        impl->renderView().render();
    })
        ->setOptions({
            { "title", "Subpixel Blend Limit" },
            { "min", fxaaOptions->GetSubpixelBlendLimitMinValue() },
            { "max", fxaaOptions->GetSubpixelBlendLimitMaxValue() },
            { "step", 0.1f }
    });

    fxaaOptionsGroup->addProperty<float>("SubpixelContrastThreshold",
        [impl] () { return impl->renderer(0)->GetFXAAOptions()->GetSubpixelContrastThreshold(); },
        [impl] (float threshold)
    {
        for (unsigned i = 0; i < impl->renderView().numberOfSubViews(); ++i)
        {
            impl->renderer(i)->GetFXAAOptions()->SetSubpixelContrastThreshold(threshold);
        }
        impl->renderView().render();
    })
        ->setOptions({
            { "title", "Subpixel Contrast Threshold" },
            { "min", fxaaOptions->GetSubpixelContrastThresholdMinValue() },
            { "max", fxaaOptions->GetSubpixelContrastThresholdMaxValue() },
            { "step", 0.1f }
    });

    fxaaOptionsGroup->addProperty<bool>("UseHighQualityEndpoints",
        [impl] () { return impl->renderer(0)->GetFXAAOptions()->GetUseHighQualityEndpoints(); },
        [impl] (bool useHQAlg)
    {
        for (unsigned i = 0; i < impl->renderView().numberOfSubViews(); ++i)
        {
            impl->renderer(i)->GetFXAAOptions()->SetUseHighQualityEndpoints(useHQAlg);
        }
        impl->renderView().render();
    })
        ->setOption("title", "Use High Quality Endpoints");

    fxaaOptionsGroup->addProperty<int>("EndpointSearchIterations",
        [impl] () { return impl->renderer(0)->GetFXAAOptions()->GetEndpointSearchIterations(); },
        [impl] (int numIterations)
    {
        for (unsigned i = 0; i < impl->renderView().numberOfSubViews(); ++i)
        {
            impl->renderer(i)->GetFXAAOptions()->SetEndpointSearchIterations(numIterations);
        }
        impl->renderView().render();
    })
        ->setOptions({
            { "title", "Endpoint Search Iterations" },
            { "min", fxaaOptions->GetEndpointSearchIterationsMinValue() },
            { "max", fxaaOptions->GetEndpointSearchIterationsMaxValue() }
    });
#endif
}
