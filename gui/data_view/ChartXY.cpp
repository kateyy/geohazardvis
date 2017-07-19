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

#include <gui/data_view/ChartXY.h>

#include <vtkContextMouseEvent.h>
#include <vtkIdTypeArray.h>
#include <vtkObjectFactory.h>
#include <vtkPlot.h>


vtkStandardNewMacro(ChartXY);


ChartXY::ChartXY()
    : vtkChartXY()
    , m_mouseMoved(false)
{

}

ChartXY::~ChartXY() = default;

vtkPlot * ChartXY::selectedPlot()
{
    vtkPlot * result = nullptr;

    for (vtkIdType i = 0; i < this->GetNumberOfPlots(); ++i)
    {
        auto plot = this->GetPlot(i);

        if (plot->GetSelection() && plot->GetSelection()->GetNumberOfValues() > 0)
        {
            result = plot;
            break;
        }
    }

    return result;
}

bool ChartXY::MouseButtonPressEvent(const vtkContextMouseEvent & mouse)
{
    m_mouseMoved = false;

    return vtkChartXY::MouseButtonPressEvent(mouse);
}

bool ChartXY::MouseMoveEvent(const vtkContextMouseEvent & mouse)
{
    m_mouseMoved = true;

    return vtkChartXY::MouseMoveEvent(mouse);
}

bool ChartXY::MouseButtonReleaseEvent(const vtkContextMouseEvent & mouse)
{
    // single selection, equivalent with picking events in RenderViewStrategy subclasses
    bool singleSelectionEvent = !m_mouseMoved && (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON);

    if (!singleSelectionEvent)
    {
        return vtkChartXY::MouseButtonReleaseEvent(mouse);
    }

    // clear previous selection first (as for RenderViewStrategy)
    for (vtkIdType i = 0; i < this->GetNumberOfPlots(); ++i)
    {
        this->GetPlot(i)->SetSelection(nullptr);
    }

    auto staticClick = mouse;
    staticClick.SetButton(this->Actions.Select());

    auto retVal = vtkChartXY::MouseButtonReleaseEvent(staticClick);

    this->InvokeEvent(PlotSelectedEvent, reinterpret_cast<void *>(selectedPlot()));

    return retVal;
}
