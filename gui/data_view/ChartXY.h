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
