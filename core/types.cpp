#include <core/types.h>

#include <cassert>

#include <core/AbstractVisualizedData.h>


namespace
{

DataSelection::Indices_t genIndexArrayOrEmpty(vtkIdType index)
{
    DataSelection::Indices_t indices;

    if (index >= 0)
    {
        indices = { index };
    }

    return indices;
}

}


DataSelection::DataSelection(DataObject * dataObject)
    : DataSelection(dataObject, IndexType::invalid, Indices_t{})
{
}

DataSelection::DataSelection(DataObject * dataObject, IndexType indexType, Indices_t indices)
    : dataObject{ dataObject }
    , indexType{ indexType }
    , indices{ indices }
{
}

DataSelection::DataSelection(DataObject * dataObject, IndexType indexType, vtkIdType index)
    : DataSelection(dataObject, indexType, genIndexArrayOrEmpty(index))
{
}

DataSelection::DataSelection(const VisualizationSelection & visSelection)
    : DataSelection(visSelection.visualization
        ? &visSelection.visualization->dataObject()
        : nullptr,
        visSelection.indexType,
        visSelection.indices)
{
}

DataSelection & DataSelection::operator=(const VisualizationSelection & visSelection)
{
    *this = DataSelection(visSelection);
    return *this;
}

DataSelection::~DataSelection() = default;

bool DataSelection::isEmpty() const
{
    return !dataObject;
}

bool DataSelection::isIndexListEmpty() const
{
    return isEmpty()
        || indexType == IndexType::invalid
        || indices.empty();
}

bool DataSelection::operator==(const DataSelection & other) const
{
    return
        dataObject == other.dataObject
        && ((isIndexListEmpty() && other.isIndexListEmpty())
            || (indexType == other.indexType && indices == other.indices));
}

bool DataSelection::operator!=(const DataSelection & other) const
{
    return !(*this == other);
}

bool DataSelection::operator==(const VisualizationSelection & visSelection) const
{
    return
        dataObject == &visSelection.visualization->dataObject()
        && ((isIndexListEmpty() && visSelection.isIndexListEmpty())
            || (indexType == visSelection.indexType && indices == visSelection.indices));
}

bool DataSelection::operator!=(const VisualizationSelection & visSelection) const
{
    return !(*this == visSelection);
}

void DataSelection::clear()
{
    *this = DataSelection();
}


VisualizationSelection::VisualizationSelection(AbstractVisualizedData * visualization)
    : VisualizationSelection(visualization,
        -1,
        IndexType::invalid,
        Indices_t{})
{
}

VisualizationSelection::VisualizationSelection(
    AbstractVisualizedData * visualization, int visOutputPort, IndexType indexType, Indices_t indices)
    : visualization{ visualization }
    , visOutputPort{ visOutputPort }
    , indexType{ indexType }
    , indices{ indices }
{
}

VisualizationSelection::VisualizationSelection(
    AbstractVisualizedData * visualization, int visOutputPort, IndexType indexType, vtkIdType index)
    : VisualizationSelection(visualization, visOutputPort, indexType, genIndexArrayOrEmpty(index))
{
}

VisualizationSelection::VisualizationSelection(const DataSelection & dataSelection,
    AbstractVisualizedData * visualization, int visOutputPort)
    : VisualizationSelection(
        visualization, visOutputPort,
        dataSelection.indexType,
        dataSelection.indices)
{
    assert(&visualization->dataObject() == dataSelection.dataObject);
}

VisualizationSelection::~VisualizationSelection() = default;

bool VisualizationSelection::isEmpty() const
{
    return !visualization;
}

bool VisualizationSelection::isIndexListEmpty() const
{
    return isEmpty()
        || indexType == IndexType::invalid
        || indices.empty();
}

bool VisualizationSelection::operator==(const DataSelection & dataSelection) const
{
    return dataSelection == *this;
}

bool VisualizationSelection::operator!=(const DataSelection & dataSelection) const
{
    return !(*this == dataSelection);
}

bool VisualizationSelection::operator==(const VisualizationSelection & other) const
{
    return
        visualization == other.visualization
        && visOutputPort == other.visOutputPort
        && ((isIndexListEmpty() && other.isIndexListEmpty())
            || (indexType == other.indexType && indices == other.indices));
}

bool VisualizationSelection::operator!=(const VisualizationSelection & other) const
{
    return !(*this == other);
}

void VisualizationSelection::clear()
{
    *this = VisualizationSelection();
}
