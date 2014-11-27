#include "RenderedData.h"

#include <cassert>

#include <vtkLookupTable.h>
#include <vtkPropCollection.h>
#include <vtkInformationIntegerPointerKey.h>


RenderedData::RenderedData(DataObject * dataObject)
    : AbstractVisualizedData(dataObject)
    , m_scalars(nullptr)
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

void RenderedData::scalarsForColorMappingChangedEvent()
{
}

void RenderedData::colorMappingGradientChangedEvent()
{
}

void RenderedData::setScalarsForColorMapping(ScalarsForColorMapping * scalars)
{
    if (scalars == m_scalars)
        return;

    m_scalars = scalars;

    scalarsForColorMappingChangedEvent();
}

void RenderedData::setColorMappingGradient(vtkScalarsToColors * gradient)
{
    if (m_gradient == gradient)
        return;

    m_gradient = gradient;

    colorMappingGradientChangedEvent();
}
