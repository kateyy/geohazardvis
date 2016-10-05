#pragma once

#include <QString>

#include <core/context2D_data/Context2DData.h>


class vtkPlot;

class DataProfile2DDataObject;


/**
    Line plot for an image data profile.

    This class expects to find its scalars (named by scalarsName parameter in constructor) in the plot lines point data.
*/
class CORE_API DataProfile2DContextPlot : public Context2DData
{
public:
    explicit DataProfile2DContextPlot(DataProfile2DDataObject & dataObject);
    ~DataProfile2DContextPlot() override;

    std::unique_ptr<reflectionzeug::PropertyGroup> createConfigGroup() override;

    DataProfile2DDataObject & profileData();
    const DataProfile2DDataObject & profileData() const;

    const QString & title() const;
    void setTitle(const QString & title);

protected:
    vtkSmartPointer<vtkPlotCollection> fetchPlots() override;

    DataBounds updateVisibleBounds() override;

private:
    void updatePlot();
    void setPlotIsValid(bool isValid);

private:
    vtkSmartPointer<vtkPlot> m_plotLine;

    QString m_title;

private:
    Q_DISABLE_COPY(DataProfile2DContextPlot)
};
