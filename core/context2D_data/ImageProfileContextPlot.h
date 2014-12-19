#pragma once

#include <core/context2D_data/Context2DData.h>


class vtkPlot;

class ImageProfileData;


class ImageProfileContextPlot : public Context2DData
{
public:
    ImageProfileContextPlot(ImageProfileData * dataObject);

    reflectionzeug::PropertyGroup * createConfigGroup() override;

protected:
    vtkSmartPointer<vtkPlotCollection> fetchPlots() override;

private:
    void updatePlot();

private:
    vtkSmartPointer<vtkPlot> m_plotLine;
};
