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

#include "PlotPointsAndLine.h"

#include <vtkContext2D.h>
#include <vtkObjectFactory.h>
#include <vtkPen.h>


vtkStandardNewMacro(PlotPointsAndLine);


vtkPen * PlotPointsAndLine::GetLinePen()
{
    return this->LinePen;
}

void PlotPointsAndLine::SetLinePen(vtkPen * linePen)
{
    if (!linePen)
    {
        // initialize with new "default" pen
        this->LinePen = vtkSmartPointer<vtkPen>::New();
        return;
    }

    this->LinePen = linePen;
}

bool PlotPointsAndLine::Paint(vtkContext2D * painter)
{
    // Draw marker and lines.
    // This function partly overrides the behavior of vtkPlotLine:
    // * Markers are rendered as implemented in vtkPlotPoints
    // * For line rendering, the vtkPlotLine implementation is used, while temporarily setting
    //   the values of this->Pen

    vtkDebugMacro(<< "Paint event called in PlotPointsAndLine.");

    if (!this->Visible || !this->Points)
    {
        return false;
    }

    // Adjust this->Pen and MarkerStyle locally here, and guarantee that it will be reset as expected
    struct SetResetFlags
    {
        SetResetFlags(vtkSmartPointer<vtkPen> & penToAdjust, vtkPen * referencePen, int & markerStyle)
            : penToAdjust{ &penToAdjust }
            , referencePen{ referencePen }
            , originalPenPtr{ penToAdjust }
            , markerStyle{ markerStyle }
            , originalMarkerStyle{ markerStyle }
        {
            penToAdjust = referencePen;
        }
        ~SetResetFlags()
        {
            reset();
        }
        void reset()
        {
            if (!penToAdjust)
            {
                return;
            }
            *penToAdjust = originalPenPtr;
            markerStyle = originalMarkerStyle;
            // reset the pointer to the reference pointer -> marking that reset() was called
            penToAdjust = nullptr;
            referencePen = nullptr;
            originalPenPtr = nullptr;
        }
    private:
        vtkSmartPointer<vtkPen> * penToAdjust;
        vtkPen * referencePen;
        vtkSmartPointer<vtkPen> originalPenPtr;
        int & markerStyle;
        int originalMarkerStyle;
    };
    SetResetFlags flagsSetReset(
        this->Pen, this->LinePen,
        this->MarkerStyle);

    // First Paint call: paint only the line with its requested color and opacity.
    // Skip drawing of point markers here.
    this->MarkerStyle = vtkPlotPoints::NONE;

    if (!this->vtkPlotLine::Paint(painter))
    {
        return false;
    }

    // Now render the point markers with the original configuration:
    flagsSetReset.reset();
    return this->vtkPlotPoints::Paint(painter);
}

bool PlotPointsAndLine::PaintLegend(vtkContext2D * painter, const vtkRectf & rect, int legendIndex)
{
    // Draw line similar to the implementation 
    painter->ApplyPen(this->LinePen);
    painter->DrawLine(rect[0], rect[1] + 0.5*rect[3],
        rect[0] + rect[2], rect[1] + 0.5*rect[3]);

    if (!this->vtkPlotPoints::PaintLegend(painter, rect, legendIndex))
    {
        return false;
    }
    painter->ApplyPen(this->Pen);
    painter->DrawLine(rect[0], rect[1] + 0.5*rect[3],
        rect[0] + rect[2], rect[1] + 0.5*rect[3]);
    this->Superclass::PaintLegend(painter, rect, 0);
    return true;
}

PlotPointsAndLine::PlotPointsAndLine()
    : Superclass()
    , LinePen{ vtkSmartPointer<vtkPen>::New() }
{
    LinePen->DeepCopy(this->Pen);
}

PlotPointsAndLine::~PlotPointsAndLine() = default;
