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

        if (plot->GetSelection() && plot->GetSelection()->GetSize() > 0)
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
