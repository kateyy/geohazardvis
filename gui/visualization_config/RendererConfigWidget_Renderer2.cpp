/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "RendererConfigWidget.h"

#include <cassert>

#include <vtkLightKit.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/utility/GridAxes3DActor.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>


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
            [renderView] () { return renderView->axesEnabled(); },
            [renderView] (bool enabled) {
            renderView->setEnableAxes(enabled);
            renderView->render();
        });

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
            [impl] () { return impl->axesActor(0)->GetLabelsVisible(); },
            [impl, renderView] (bool visible) {
            for (unsigned int i = 0; i < renderView->numberOfSubViews(); ++i)
            {
                impl->axesActor(i)->SetLabelsVisible(visible);
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
