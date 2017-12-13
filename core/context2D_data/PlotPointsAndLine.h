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

#include <vtkPlotLine.h>
#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkPen;


class CORE_API PlotPointsAndLine : public vtkPlotLine
{
public:
    static PlotPointsAndLine * New();
    vtkTypeMacro(PlotPointsAndLine, vtkPlotLine);

    vtkPen * GetLinePen();
    void SetLinePen(vtkPen * linePen);

    bool Paint(vtkContext2D * painter) override;

    bool PaintLegend(vtkContext2D * painter, const vtkRectf & rect,
        int legendIndex) override;

protected:
    PlotPointsAndLine();
    ~PlotPointsAndLine() override;

private:
    vtkSmartPointer<vtkPen> LinePen;

private:
    PlotPointsAndLine(const PlotPointsAndLine&) = delete;
    void operator=(const PlotPointsAndLine&) = delete;
};
