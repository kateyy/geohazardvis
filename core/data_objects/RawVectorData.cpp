#include "RawVectorData.h"

#include <vtkFloatArray.h>
#include <vtkEventQtSlotConnect.h>

#include <core/table_model/QVtkTableModelRawVector.h>


RawVectorData::RawVectorData(QString name, vtkFloatArray * dataArray)
    : DataObject(name, nullptr)
    , m_dataArray(dataArray)
{
    dataArray->SetName(name.toLatin1().data());

    vtkQtConnect()->Connect(dataArray, vtkCommand::ModifiedEvent, this, SLOT(_dataChanged()));
}

RawVectorData::~RawVectorData() = default;

bool RawVectorData::is3D() const
{
    return false;
}

RenderedData * RawVectorData::createRendered()
{
    return nullptr;
}

QString RawVectorData::dataTypeName() const
{
    static QString name = "raw vector";
    return name;
}

vtkFloatArray * RawVectorData::dataArray()
{
    return m_dataArray;
}

QVtkTableModel * RawVectorData::createTableModel()
{
    QVtkTableModel * model = new QVtkTableModelRawVector;
    model->setDataObject(this);

    return model;
}
