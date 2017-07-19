/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <core/types.h>

#include <cassert>
#include <ostream>
#include <string>

#include <QDebug>

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

std::string toString(ContentType contentType)
{
    switch (contentType)
    {
    case ContentType::Rendered3D: return "Rendered3D";
    case ContentType::Rendered2D: return "Rendered2D";
    case ContentType::Context2D: return "Context2D";
    case ContentType::invalid: return "invalid";
    default: return "[invalid value: " + std::to_string(static_cast<int>(contentType)) + "]";
    }
}

std::string toString(IndexType indexType)
{
    switch (indexType)
    {
    case IndexType::points: return "points";
    case IndexType::cells: return "cells";
    case IndexType::invalid: return "invalid";
    default: return "[invalid value: " + std::to_string(static_cast<int>(indexType)) + "]";
    }
}

}


QDebug & operator<<(QDebug & qdebug, ContentType contentType)
{
    qdebug.noquote().nospace() << QString::fromStdString(toString(contentType));
    return qdebug.maybeQuote().maybeSpace();
}

std::ostream & operator<<(std::ostream & stream, ContentType contentType)
{
    stream << toString(contentType);
    return stream;
}

QDebug & operator<<(QDebug & qdebug, IndexType indexType)
{
    qdebug.noquote().nospace() << QString::fromStdString(toString(indexType));
    return qdebug.maybeQuote().maybeSpace();
}

std::ostream & operator<<(std::ostream & stream, IndexType indexType)
{
    stream << toString(indexType);
    return stream;
}


DataSelection::DataSelection(DataObject * dataObject)
    : DataSelection(dataObject, IndexType::invalid, Indices_t{})
{
}

DataSelection::DataSelection(DataObject * dataObject, IndexType indexType, Indices_t indices)
    : dataObject{ dataObject }
    , indexType{ indexType }
    , indices{ std::move(indices) }
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

void DataSelection::setIndex(vtkIdType index)
{
    if (index != -1)
    {
        indices = { index };
    }
    else
    {
        indices.clear();
    }
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
    , indices{ std::move(indices) }
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

void VisualizationSelection::setIndex(vtkIdType index)
{
    if (index != -1)
    {
        indices = { index };
    }
    else
    {
        indices.clear();
    }
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
