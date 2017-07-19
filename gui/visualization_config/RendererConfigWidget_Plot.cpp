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

#include <vtkAxis.h>
#include <vtkChartLegend.h>
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkTextProperty.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/types.h>
#include <gui/data_view/RendererImplementationPlot.h>


using namespace reflectionzeug;

namespace
{
enum class SelectionMode
{
    none = vtkContextScene::SELECTION_NONE,
    default_ = vtkContextScene::SELECTION_DEFAULT,
    addition = vtkContextScene::SELECTION_ADDITION,
    subtraction = vtkContextScene::SELECTION_SUBTRACTION,
    toggle = vtkContextScene::SELECTION_TOGGLE
};
enum class Notation
{
    standard = vtkAxis::STANDARD_NOTATION,
    scietific = vtkAxis::SCIENTIFIC_NOTATION,
    mixed = vtkAxis::FIXED_NOTATION
};
}


std::unique_ptr<PropertyGroup> RendererConfigWidget::createPropertyGroupPlot(
    AbstractRenderView * /*renderView*/, RendererImplementationPlot * impl)
{
    auto root = std::make_unique<PropertyGroup>();

    root->addProperty<bool>("AxesAutoUpdate", impl, &RendererImplementationPlot::axesAutoUpdate, &RendererImplementationPlot::setAxesAutoUpdate)
        ->setOption("title", "Automatic Axes Update");


    auto addAxisProperties = [impl, &root] (int axis, const std::string & groupName, const std::string groupTitle)
    {
        auto group = root->addGroup(groupName);
        group->setOption("title", groupTitle);

        group->addProperty<std::string>("Label",
            [impl, axis] () { return impl->chart()->GetAxis(axis)->GetTitle(); },
            [impl, axis] (const std::string & label) {
            impl->chart()->GetAxis(axis)->SetTitle(label);
            impl->render();
        });

        auto prop_fontSize = group->addProperty<int>("LabelFontSize",
            [impl, axis] () { return impl->chart()->GetAxis(axis)->GetTitleProperties()->GetFontSize(); },
            [impl, axis] (int size) {
            impl->chart()->GetAxis(axis)->GetTitleProperties()->SetFontSize(size);
            impl->render();
        });
        prop_fontSize->setOption("title", "Label Font Size");
        prop_fontSize->setOption("minimum", impl->chart()->GetAxis(axis)->GetTitleProperties()->GetFontSizeMinValue());
        prop_fontSize->setOption("maximum", impl->chart()->GetAxis(axis)->GetTitleProperties()->GetFontSizeMaxValue());

        auto prop_tickFontSize = group->addProperty<int>("TickFontSize",
            [impl, axis] () { return impl->chart()->GetAxis(axis)->GetLabelProperties()->GetFontSize(); },
            [impl, axis] (int size) {
            impl->chart()->GetAxis(axis)->GetLabelProperties()->SetFontSize(size);
            impl->render();
        });
        prop_tickFontSize->setOption("title", "Tick Font Size");
        prop_tickFontSize->setOption("minimum", impl->chart()->GetAxis(axis)->GetLabelProperties()->GetFontSizeMinValue());
        prop_tickFontSize->setOption("maximum", impl->chart()->GetAxis(axis)->GetLabelProperties()->GetFontSizeMaxValue());

        auto prop_precision = group->addProperty<int>("Precision",
            [impl, axis] () { return impl->chart()->GetAxis(axis)->GetPrecision(); },
            [impl, axis] (int p) {
            impl->chart()->GetAxis(axis)->SetPrecision(p);
            impl->render();
        });
        prop_precision->setOption("minimum", 0);
        prop_precision->setOption("maximum", 100);

        auto prop_notation = group->addProperty<Notation>("Notation",
            [impl, axis] () { return static_cast<Notation>(impl->chart()->GetAxis(axis)->GetNotation()); },
            [impl, axis] (Notation n) { 
            impl->chart()->GetAxis(axis)->SetNotation(static_cast<int>(n));
            impl->render();
        });
        prop_notation->setStrings({
                { Notation::standard, "standard" },
                { Notation::scietific, "scientific" },
                { Notation::mixed, "mixed" }
        });
    };

    addAxisProperties(vtkAxis::BOTTOM, "xAxis", "x-Axis");
    addAxisProperties(vtkAxis::LEFT, "yAxis", "y-Axis");

    root->addProperty<bool>("ChartLegend",
        [impl] () { return impl->chart()->GetShowLegend(); },
        [impl] (bool v) { 
        impl->chart()->SetShowLegend(v);
        impl->render();
    })
        ->setOption("title", "Chart Legend");

    impl->chart()->GetLegend()->SetVerticalAlignment(vtkChartLegend::BOTTOM);
    root->addProperty<bool>("ChartLegendOnTop",
    [impl] ()
    {
        return impl->chart()->GetLegend()->GetVerticalAlignment() == vtkChartLegend::TOP;
    },
    [impl] (bool showOnTop)
    {
        impl->chart()->GetLegend()->SetVerticalAlignment(showOnTop
            ? vtkChartLegend::TOP : vtkChartLegend::BOTTOM);
        impl->render();
    })
        ->setOption("title", "Legend on top");

    //auto backgroundColor = root->addProperty<Color>("backgroundColor",
    //    [impl] () {
    //    unsigned char * color = impl->chart()->GetBackgroundBrush()->GetColor();
    //    return Color(color[0], color[1], color[2]);
    //},
    //    [impl] (const Color & color) {

    //    impl->chart()->GetBackgroundBrush()->SetColor(color.red(), color.green(), color.blue());
    //    impl->render();
    //});
    //backgroundColor->setOption("title", "Background Color");

    //auto prop_selectionMode = root->addProperty<SelectionMode>("SelectionMode",
    //    [impl] () { return static_cast<SelectionMode>(impl->chart()->GetSelectionMode()); },
    //    [impl] (SelectionMode mode) {
    //    impl->chart()->SetSelectionMode(static_cast<int>(mode));
    //    impl->render();
    //});
    //prop_selectionMode->setStrings({
    //        { SelectionMode::none, "none" },
    //        { SelectionMode::default_, "default" },
    //        { SelectionMode::addition, "addition" },
    //        { SelectionMode::subtraction, "subtraction" },
    //        { SelectionMode::toggle, "toggle" } });

    return root;
}
