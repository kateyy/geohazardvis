#include "AbstractVisualizedData.h"

#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkDataSet.h>
#include <vtkLookupTable.h>

#include <core/AbstractVisualizedData_private.h>
#include <core/data_objects/DataObject.h>
#include <core/utility/macros.h>


AbstractVisualizedData::AbstractVisualizedData(ContentType contentType, DataObject & dataObject)
    : QObject()
    , d_ptr{ std::make_unique<AbstractVisualizedData_private>(contentType, dataObject) }
{
    connect(&dataObject, &DataObject::dataChanged, this, &AbstractVisualizedData::geometryChanged);
}

AbstractVisualizedData::~AbstractVisualizedData()
{
    if (d_ptr->colorMapping)
    {
        d_ptr->colorMapping->unregisterVisualizedData(this);
    }
}

ContentType AbstractVisualizedData::contentType() const
{
    return d_ptr->contentType;
}

DataObject & AbstractVisualizedData::dataObject()
{
    return d_ptr->dataObject;
}

const DataObject & AbstractVisualizedData::dataObject() const
{
    return d_ptr->dataObject;
}

bool AbstractVisualizedData::isVisible() const
{
    return d_ptr->isVisible;
}

void AbstractVisualizedData::setVisible(bool visible)
{
    d_ptr->isVisible = visible;

    visibilityChangedEvent(visible);

    emit visibilityChanged(visible);
}

const DataBounds & AbstractVisualizedData::visibleBounds()
{
    if (!d_ptr->visibleBoundsAreValid())
    {
        d_ptr->validateVisibleBounds(updateVisibleBounds());
    }

    return d_ptr->visibleBounds();
}

ColorMapping & AbstractVisualizedData::colorMapping()
{
    if (!d_ptr->colorMapping)
    {
        d_ptr->colorMapping = std::make_unique<ColorMapping>();
        d_ptr->colorMapping->registerVisualizedData(this);
        setupColorMapping(*d_ptr->colorMapping);
    }

    return *d_ptr->colorMapping;
}

void AbstractVisualizedData::setScalarsForColorMapping(ColorMappingData * scalars)
{
    if (scalars == d_ptr->colorMappingData)
    {
        return;
    }

    d_ptr->colorMappingData = scalars;

    scalarsForColorMappingChangedEvent();
}

void AbstractVisualizedData::setColorMappingGradient(vtkScalarsToColors * gradient)
{
    if (d_ptr->gradient == gradient)
    {
        return;
    }

    d_ptr->gradient = gradient;

    colorMappingGradientChangedEvent();
}

int AbstractVisualizedData::numberOfColorMappingInputs() const
{
    return 1;
}

vtkAlgorithmOutput * AbstractVisualizedData::colorMappingInput(int DEBUG_ONLY(connection))
{
    assert(connection == 0);

    return dataObject().processedOutputPort();
}

vtkDataSet * AbstractVisualizedData::colorMappingInputData(int connection)
{
    auto alg = colorMappingInput(connection)->GetProducer();
    alg->Update();
    return vtkDataSet::SafeDownCast(alg->GetOutputDataObject(0));
}

int AbstractVisualizedData::defaultVisualizationPort() const
{
    return 0;
}

void AbstractVisualizedData::setupInformation(vtkInformation & information, AbstractVisualizedData & visualization)
{
    auto & dataObject = visualization.dataObject();
    DataObject::storePointer(information, &dataObject);
    DataObject::storeName(information, dataObject);
    AbstractVisualizedData::storePointer(information, &visualization);
}

AbstractVisualizedData * AbstractVisualizedData::readPointer(vtkInformation & information)
{
    static_assert(sizeof(int*) == sizeof(DataObject*), "");

    if (information.Has(AbstractVisualizedData_private::VisualizedDataKey()))
    {
        assert(information.Length(AbstractVisualizedData_private::VisualizedDataKey()) == 1);
        return reinterpret_cast<AbstractVisualizedData *>(information.Get(AbstractVisualizedData_private::VisualizedDataKey()));
    }

    return nullptr;
}

void AbstractVisualizedData::storePointer(vtkInformation & information, AbstractVisualizedData * visualization)
{
    information.Set(AbstractVisualizedData_private::VisualizedDataKey(), reinterpret_cast<int *>(visualization), 1);
}

void AbstractVisualizedData::setupColorMapping(ColorMapping & /*colorMapping*/)
{
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

ColorMappingData * AbstractVisualizedData::currentColorMappingData()
{
    return d_ptr->colorMappingData;
}

vtkScalarsToColors * AbstractVisualizedData::currentColorMappingGradient()
{
    return d_ptr->gradient;
}

void AbstractVisualizedData::invalidateVisibleBounds()
{
    d_ptr->invalidateVisibleBounds();
}
