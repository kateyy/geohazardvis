#pragma once

#include <QString>

#include <core/context2D_data/Context2DData.h>


class vtkPlot;

class ImageProfileData;


/**
    Line plot for an image data profile.

    This class expects to find its scalars (named by scalarsName parameter in constructor) in the plot lines point data.
*/
class CORE_API ImageProfileContextPlot : public Context2DData
{
public:
    explicit ImageProfileContextPlot(ImageProfileData & dataObject);
    ~ImageProfileContextPlot() override;

    std::unique_ptr<reflectionzeug::PropertyGroup> createConfigGroup() override;

    ImageProfileData & profileData();
    const ImageProfileData & profileData() const;

    const QString & title() const;
    void setTitle(const QString & title);

protected:
    vtkSmartPointer<vtkPlotCollection> fetchPlots() override;

private:
    void updatePlot();
    void setPlotIsValid(bool isValid);

private:
    vtkSmartPointer<vtkPlot> m_plotLine;

    QString m_title;

private:
    Q_DISABLE_COPY(ImageProfileContextPlot)
};
