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

#include <core/utility/types_utils.h>

#include <algorithm>

#include <QString>

#include <vtkAssignAttribute.h>
#include <vtkDataSet.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkRearrangeFields.h>


vtkDataSetAttributes * IndexType_util::extractAttributes(vtkDataSet & dataSet) const
{
    switch (value)
    {
    case IndexType::points:
        return dataSet.GetPointData();
    case IndexType::cells:
        return dataSet.GetCellData();
    case IndexType::invalid:
    default:
        return nullptr;
    }
}

vtkDataSetAttributes * IndexType_util::extractAttributes(vtkDataSet * dataSet) const
{
    if (dataSet)
    {
        return extractAttributes(*dataSet);
    }
    return nullptr;
}

vtkDataArray * IndexType_util::extractArray(vtkDataSet & dataSet, const QString & name) const
{
    return extractArray(dataSet, name.toUtf8().data());
}

vtkDataArray * IndexType_util::extractArray(vtkDataSet & dataSet, const char * name) const
{
    if (auto attr = extractAttributes(dataSet))
    {
        return attr->GetArray(name);
    }
    return nullptr;
}

vtkAbstractArray * IndexType_util::extractAbstractArray(vtkDataSet & dataSet, const QString & name) const
{
    return extractAbstractArray(dataSet, name.toUtf8().data());
}

vtkAbstractArray * IndexType_util::extractAbstractArray(vtkDataSet & dataSet, const char * name) const
{
    if (auto attr = extractAttributes(dataSet))
    {
        return attr->GetAbstractArray(name);
    }
    return nullptr;
}

vtkDataArray * IndexType_util::extractArray(vtkDataSet * dataSet, const QString & name) const
{
    if (dataSet)
    {
        return extractArray(*dataSet, name);
    }
    return nullptr;
}

vtkDataArray * IndexType_util::extractArray(vtkDataSet * dataSet, const char * name) const
{
    if (dataSet)
    {
        return extractArray(*dataSet, name);
    }
    return nullptr;
}

vtkAbstractArray * IndexType_util::extractAbstractArray(vtkDataSet * dataSet, const QString & name) const
{
    if (dataSet)
    {
        return extractAbstractArray(*dataSet, name);
    }
    return nullptr;
}

vtkAbstractArray * IndexType_util::extractAbstractArray(vtkDataSet * dataSet, const char * name) const
{
    if (dataSet)
    {
        return extractAbstractArray(*dataSet, name);
    }
    return nullptr;
}

int IndexType_util::toVtkAssignAttribute_AttributeLocation() const
{
    switch (value)
    {
    case IndexType::points: return vtkAssignAttribute::AttributeLocation::POINT_DATA;
    case IndexType::cells: return vtkAssignAttribute::AttributeLocation::CELL_DATA;
    case IndexType::invalid:
    default:
        return -1;
    }
}

IndexType_util & IndexType_util::fromVtkAssignAttribute_AttributeLocation(int attributeLocation)
{
    switch (attributeLocation)
    {
    case vtkAssignAttribute::AttributeLocation::POINT_DATA:
        value = IndexType::points;
        break;
    case vtkAssignAttribute::AttributeLocation::CELL_DATA:
        value = IndexType::cells;
        break;
    default:
        value = IndexType::invalid;
        break;
    }
    return *this;
}

int IndexType_util::toVtkFieldAssociation() const
{
    switch (value)
    {
    case IndexType::points: return vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_POINTS;
    case IndexType::cells: return vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_CELLS;
    case IndexType::invalid:
    default:
        return -1;
    }
}

IndexType_util & IndexType_util::fromVtkFieldAssociation(int fieldAssociation)
{
    switch (fieldAssociation)
    {
    case vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_POINTS:
        value = IndexType::points;
        break;
    case vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_CELLS:
        value = IndexType::cells;
        break;
    default:
        value = IndexType::invalid;
    }
    return *this;
}

int IndexType_util::toVtkRearrangeFields_FieldLocation() const
{
    switch (value)
    {
    case IndexType::points: return vtkRearrangeFields::FieldLocation::POINT_DATA;
    case IndexType::cells: return vtkRearrangeFields::FieldLocation::CELL_DATA;
    case IndexType::invalid:
    default:
        return -1;
    }
}

IndexType_util & IndexType_util::fromVtkRearrangeFields_FieldLocation(int fieldLocation)
{
    switch (fieldLocation)
    {
    case vtkRearrangeFields::FieldLocation::POINT_DATA:
        value = IndexType::points;
        break;
    case vtkRearrangeFields::FieldLocation::CELL_DATA:
        value = IndexType::cells;
        break;
    default:
        value = IndexType::invalid;
    }
    return *this;
}
