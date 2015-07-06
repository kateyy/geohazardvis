#pragma once

#include <QString>

#include <vtkWeakPointer.h>

#include <core/context2D_data/Context2DData.h>


class vtkDataArray;
class vtkPlot;

class ImageProfileData;


/**
    Line plot for an image data profile.

    This class expects to find its scalars (named by scalarsName parameter in constructor) in the plot lines point data.
*/
class CORE_API ImageProfileContextPlot : public Context2DData
{
public:
    ImageProfileContextPlot(ImageProfileData & dataObject, const QString & scalarsName);

    reflectionzeug::PropertyGroup * createConfigGroup() override;

    const QString & title() const;
    void setTitle(const QString & title);

protected:
    vtkSmartPointer<vtkPlotCollection> fetchPlots() override;

private:
    void updatePlot();

private:
    vtkSmartPointer<vtkPlot> m_plotLine;

    const QString m_scalarsName;

    QString m_title;
};
