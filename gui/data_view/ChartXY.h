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

#pragma once

#include <vtkChartXY.h>

#include <gui/gui_api.h>


class GUI_API ChartXY : public vtkChartXY
{
public:
    vtkTypeMacro(ChartXY, vtkChartXY);

    static ChartXY * New();

    enum EventIds
    {
        // vtkChart::EventIds::UpdateRange == 1002
        PlotSelectedEvent = 1004
    };

    vtkPlot * selectedPlot();

protected:
    ChartXY();
    ~ChartXY() override;

    bool MouseButtonPressEvent(const vtkContextMouseEvent & mouse) override;
    bool MouseMoveEvent(const vtkContextMouseEvent & mouse) override;
    bool MouseButtonReleaseEvent(const vtkContextMouseEvent & mouse) override;

private:
    bool m_mouseMoved;

public:
    ChartXY(const ChartXY &) = delete;
    void operator=(const ChartXY &) = delete;
};
