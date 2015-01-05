#pragma once

#include <QString>

#include <vtkWeakPointer.h>

#include <core/context2D_data/Context2DData.h>


class vtkDataArray;
class vtkPlot;

class ImageProfileData;


class CORE_API ImageProfileContextPlot : public Context2DData
{
public:
    ImageProfileContextPlot(ImageProfileData * dataObject);

    reflectionzeug::PropertyGroup * createConfigGroup() override;

    const QString & title() const;
    void setTitle(const QString & title);

protected:
    vtkSmartPointer<vtkPlotCollection> fetchPlots() override;

private:
    void updatePlot();

private:
    vtkSmartPointer<vtkPlot> m_plotLine;
    QString m_title;
};
