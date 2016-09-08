#include "Context2DData.h"

#include <cassert>

#include <vtkPlot.h>

#include <core/types.h>
#include <core/context2D_data/vtkPlotCollection.h>


Context2DData::Context2DData(DataObject & dataObject)
    : AbstractVisualizedData(ContentType::Context2D, dataObject)
    , m_plotsInvalidated{ true }
{
}

Context2DData::~Context2DData() = default;

const vtkSmartPointer<vtkPlotCollection> & Context2DData::plots()
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

void Context2DData::visibilityChangedEvent(bool visible)
{
    vtkCollectionSimpleIterator it;
    plots()->InitTraversal(it);
    while (auto item = plots()->GetNextPlot(it))
    {
        item->SetVisible(visible);
    }
}

void Context2DData::invalidateContextItems()
{
    m_plotsInvalidated = true;

    emit plotCollectionChanged();
}
