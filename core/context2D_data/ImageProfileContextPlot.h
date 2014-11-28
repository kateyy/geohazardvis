#pragma once

#include <core/context2D_data/Context2DData.h>


class vtkChartXY;
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
    vtkSmartPointer<vtkChartXY> m_chart;
};
