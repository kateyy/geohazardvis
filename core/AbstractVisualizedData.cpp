#include "AbstractVisualizedData.h"

#include <cassert>

#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkDataSet.h>
#include <vtkLookupTable.h>

#include <core/data_objects/DataObject.h>


AbstractVisualizedData::AbstractVisualizedData(ContentType contentType, DataObject * dataObject, QObject * parent)
    : QObject(parent)
    , m_scalars(nullptr)
    , m_contentType(contentType)
    , m_dataObject(dataObject)
    , m_isVisible(true)
{
    assert(m_dataObject);

    connect(dataObject, &DataObject::dataChanged, this, &AbstractVisualizedData::geometryChanged);
}

ContentType AbstractVisualizedData::contentType() const
{
    return m_contentType;
}

DataObject * AbstractVisualizedData::dataObject()
{
    return m_dataObject;
}

const DataObject * AbstractVisualizedData::dataObject() const
{
    return m_dataObject;
}

bool AbstractVisualizedData::isVisible() const
{
    return m_isVisible;
}

void AbstractVisualizedData::setVisible(bool visible)
{
    m_isVisible = visible;

    visibilityChangedEvent(visible);

    emit visibilityChanged(visible);
}

void AbstractVisualizedData::setScalarsForColorMapping(ColorMappingData * scalars)
{
    if (scalars == m_scalars)
        return;

    m_scalars = scalars;

    scalarsForColorMappingChangedEvent();
}

void AbstractVisualizedData::setColorMappingGradient(vtkScalarsToColors * gradient)
{
    if (m_gradient == gradient)
        return;

    m_gradient = gradient;

    colorMappingGradientChangedEvent();
}

vtkAlgorithmOutput * AbstractVisualizedData::colorMappingInput()
{
    return dataObject()->processedOutputPort();
}

vtkDataSet * AbstractVisualizedData::colorMappingInputData()
{
    auto alg = colorMappingInput()->GetProducer();
    alg->Update();
    return vtkDataSet::SafeDownCast(alg->GetOutputDataObject(0));
}

void AbstractVisualizedData::visibilityChangedEvent(bool /*visible*/)
{
}

void AbstractVisualizedData::scalarsForColorMappingChangedEvent()
{
}

void AbstractVisualizedData::colorMappingGradientChangedEvent()
{
}
