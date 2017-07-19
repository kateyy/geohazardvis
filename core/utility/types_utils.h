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

#pragma once

#include <core/types.h>


class QString;
class vtkAbstractArray;
class vtkDataArray;
class vtkDataSet;
class vtkDataSetAttributes;


/**
 * Extract data set attributes by location and name.
 *
 * IndexType is used in many contexts to define the association/location of an attribute in a data
 * set (points/cells). This utility class simplifies retrieval of attributes based on a IndexType.
 *
 * Furthermore, conversions from various enums defining attribute association/location in VTK
 * classes to IndexType are provided here. These enums usually support more locations than
 * currently targeted by IndexType (e.g., vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS).
 * These are mapped to IndexType::invalid, as they have no meaning in implementations that use
 * IndexType.
 */
struct CORE_API IndexType_util
{
    IndexType value;

    explicit IndexType_util(IndexType value = IndexType::invalid) : value{ value } { }

    operator IndexType() const { return value; }

    /** @return point data or cell data of dataSet, or nullptr if value is IndexType::invalid. */
    vtkDataSetAttributes * extractAttributes(vtkDataSet & dataSet) const;
    vtkDataSetAttributes * extractAttributes(vtkDataSet * dataSet) const;

    /** 
     * Extract an attribute array named by name from dataSet's point or cell data, depending on
     * the current IndexType value.
     */
    vtkDataArray * extractArray(vtkDataSet & dataSet, const QString & name) const;
    vtkDataArray * extractArray(vtkDataSet & dataSet, const char * name) const;
    vtkAbstractArray * extractAbstractArray(vtkDataSet & dataSet, const QString & name) const;
    vtkAbstractArray * extractAbstractArray(vtkDataSet & dataSet, const char * name) const;

    vtkDataArray * extractArray(vtkDataSet * dataSet, const QString & name) const;
    vtkDataArray * extractArray(vtkDataSet * dataSet, const char * name) const;
    vtkAbstractArray * extractAbstractArray(vtkDataSet * dataSet, const QString & name) const;
    vtkAbstractArray * extractAbstractArray(vtkDataSet * dataSet, const char * name) const;

    /**
     * @return the related vtkAssignAttribute::AttributeLocation enum value.
     * If value is invalid, return -1
     */
    int toVtkAssignAttribute_AttributeLocation() const;
    /**
     * Set the value from vtkAssignAttribute::AttributeLocation enum value, if supported.
     * Otherwise, value is set the invalid.
     */
    IndexType_util & fromVtkAssignAttribute_AttributeLocation(int attributeLocation);

    /**
     * @return related vtkDataObject::FieldAssociations enum value.
     * If values is invalid, return -1
     */
    int toVtkFieldAssociation() const;
    /**
     * Set the value from vtkDataObject::FieldAssociations enum value, if supported.
     * Otherwise, value is set the invalid.
     */
    IndexType_util & fromVtkFieldAssociation(int fieldAssociation);

    int toVtkRearrangeFields_FieldLocation() const;
    IndexType_util & fromVtkRearrangeFields_FieldLocation(int fieldLocation);

    bool operator==(IndexType_util rhs) const { return this->value == rhs.value; }
    bool operator==(IndexType rhs) const { return this->value == rhs; }
    bool operator!=(IndexType_util rhs) const { return this->value != rhs.value; }
    bool operator!=(IndexType rhs) const { return this->value != rhs; }

    IndexType_util(const IndexType_util &) = default;
    IndexType_util(IndexType_util &&) = default;
    IndexType_util & operator=(const IndexType_util &) = default;
    IndexType_util & operator=(IndexType_util &&) = default;
};

inline bool operator==(IndexType lhs, IndexType_util rhs) { return lhs == rhs.value; }
inline bool operator!=(IndexType lhs, IndexType_util rhs) { return lhs != rhs.value; }
