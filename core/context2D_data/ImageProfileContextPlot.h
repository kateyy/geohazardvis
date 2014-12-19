#pragma once

#include <core/context2D_data/Context2DData.h>


class vtkChartXY;
class vtkPlot;

class ImageProfileData;


class ImageProfileContextPlot : public Context2DData
{
public:
    ImageProfileContextPlot(ImageProfileData * dataObject);

    reflectionzeug::PropertyGroup * createConfigGroup() override;

protected:
    vtkSmartPointer<vtkContextItemCollection> fetchContextItems() override;

    void visibilityChangedEvent(bool visible) override;

private:
    void updatePlot();

private:
    vtkSmartPointer<vtkChartXY> m_chart;
    vtkSmartPointer<vtkPlot> m_plotLine;
};
