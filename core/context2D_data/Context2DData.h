#pragma once

#include <vtkSmartPointer.h>

#include <core/AbstractVisualizedData.h>


class vtkPlotCollection;


class CORE_API Context2DData : public AbstractVisualizedData
{
    Q_OBJECT

public:
    Context2DData(DataObject * dataObject);

    vtkSmartPointer<vtkPlotCollection> plots();

protected:
    virtual vtkSmartPointer<vtkPlotCollection> fetchPlots() = 0;
    void invalidateContextItems();

signals:
    void plotCollectionChanged();

private:
    vtkSmartPointer<vtkPlotCollection> m_plots;
    bool m_plotsInvalidated;
};
