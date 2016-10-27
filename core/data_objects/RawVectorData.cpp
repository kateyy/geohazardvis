#include "RawVectorData.h"

#include <vtkCommand.h>
#include <vtkFloatArray.h>

#include <core/types.h>
#include <core/table_model/QVtkTableModelRawVector.h>


RawVectorData::RawVectorData(const QString & name, vtkFloatArray & dataArray)
    : DataObject(name, nullptr)
    , m_dataArray{ &dataArray }
{
    dataArray.SetName(name.toUtf8().data());

    connectObserver("dataChanged", dataArray, vtkCommand::ModifiedEvent, *this, &RawVectorData::_dataChanged);
}

RawVectorData::~RawVectorData() = default;

bool RawVectorData::is3D() const
{
    return false;
}

IndexType RawVectorData::defaultAttributeLocation() const
{
    return IndexType::invalid;
}

const QString & RawVectorData::dataTypeName() const
{
    return dataTypeName_s();
}

const QString & RawVectorData::dataTypeName_s()
{
    static QString name{ "Raw Data Vector" };
    return name;
}

vtkFloatArray * RawVectorData::dataArray()
{
    return m_dataArray;
}

std::unique_ptr<QVtkTableModel> RawVectorData::createTableModel()
{
    std::unique_ptr<QVtkTableModel> model = std::make_unique<QVtkTableModelRawVector>();
    model->setDataObject(this);

    return model;
}
