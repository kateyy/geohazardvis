#include "AttributeVectorData.h"

#include <vtkFloatArray.h>

#include <core/QVtkTableModel.h>


AttributeVectorData::AttributeVectorData(QString name, vtkFloatArray * dataArray)
    : DataObject(name, nullptr)
    , m_dataArray(dataArray)
{
}

AttributeVectorData::~AttributeVectorData() = default;

RenderedData * AttributeVectorData::createRendered()
{
    return nullptr;
}

QString AttributeVectorData::dataTypeName() const
{
    static QString name = "attribute vector";
    return name;
}

vtkFloatArray * AttributeVectorData::dataArray()
{
    return m_dataArray;
}

QVtkTableModel * AttributeVectorData::createTableModel()
{
    QVtkTableModel * model = new QVtkTableModel;

    return model;
}
