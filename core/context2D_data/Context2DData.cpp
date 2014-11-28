#include "Context2DData.h"

#include <cassert>

#include <core/context2D_data/vtkContextItemCollection.h>


Context2DData::Context2DData(DataObject * dataObject)
    : AbstractVisualizedData(dataObject)
    , m_contextItemsInvalid(true)
{
}

vtkSmartPointer<vtkContextItemCollection> Context2DData::contextItems()
{
    if (m_contextItemsInvalid)
    {
        m_contextItemsInvalid = false;
        m_viewProps = fetchContextItems();
    }

    assert(m_viewProps);
    assert(m_viewProps->GetNumberOfItems() > 0);

    return m_viewProps;
}

void Context2DData::invalidateContextItems()
{
    m_contextItemsInvalid = true;

    emit contextItemCollectionChanged();
}
