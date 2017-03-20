#pragma once

#include <core/types.h>


class QString;
class vtkAbstractArray;
class vtkDataArray;
class vtkDataSet;
class vtkDataSetAttributes;


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
