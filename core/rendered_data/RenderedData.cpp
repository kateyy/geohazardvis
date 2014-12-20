#include "RenderedData.h"

#include <cassert>

#include <vtkPropCollection.h>


RenderedData::RenderedData(ContentType contentType, DataObject * dataObject)
    : AbstractVisualizedData(contentType, dataObject)
    , m_viewPropsInvalid(true)
{
}

vtkSmartPointer<vtkPropCollection> RenderedData::viewProps()
{
    if (m_viewPropsInvalid)
    {
        m_viewPropsInvalid = false;
        m_viewProps = fetchViewProps();
    }

    assert(m_viewProps);
    assert(m_viewProps->GetNumberOfItems() > 0);

    return m_viewProps;
}

void RenderedData::invalidateViewProps()
{
    m_viewPropsInvalid = true;

    emit viewPropCollectionChanged();
}
