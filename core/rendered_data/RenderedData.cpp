#include "RenderedData.h"

#include <cassert>

#include <vtkLookupTable.h>
#include <vtkPropCollection.h>
#include <vtkInformationIntegerPointerKey.h>

#include <core/data_objects/DataObject.h>


RenderedData::RenderedData(DataObject * dataObject)
    : QObject()
    , m_dataObject(dataObject)
    , m_scalars(nullptr)
    , m_viewPropsInvalid(true)
    , m_isVisible(true)
{
    connect(dataObject, &DataObject::dataChanged, this, &RenderedData::geometryChanged);
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

bool RenderedData::isVisible() const
{
    return m_isVisible;
}

void RenderedData::setVisible(bool visible)
{
    m_isVisible = visible;

    visibilityChangedEvent(visible);

    emit visibilityChanged(visible);
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

void RenderedData::visibilityChangedEvent(bool /*visible*/)
{
}

DataObject * RenderedData::dataObject()
{
    return m_dataObject;
}

const DataObject * RenderedData::dataObject() const
{
    return m_dataObject;
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
