#include "Context2DData.h"

#include <cassert>

#include <core/context2D_data/vtkPlotCollection.h>


Context2DData::Context2DData(DataObject * dataObject)
    : AbstractVisualizedData(dataObject)
    , m_plotsInvalidated(true)
{
}

vtkSmartPointer<vtkPlotCollection> Context2DData::plots()
{
    if (m_plotsInvalidated)
    {
        m_plotsInvalidated = false;
        m_plots = fetchPlots();
    }

    assert(m_plots);
    assert(m_plots->GetNumberOfItems() > 0);

    return m_plots;
}

void Context2DData::invalidateContextItems()
{
    m_plotsInvalidated = true;

    emit plotCollectionChanged();
}
