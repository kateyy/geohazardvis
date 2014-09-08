#pragma once

#include <core/data_objects/DataObject.h>


class vtkFloatArray;


class CORE_API AttributeVectorData : public DataObject
{
public:
    AttributeVectorData(QString name, vtkFloatArray * dataArray);
    ~AttributeVectorData() override;

    RenderedData * createRendered() override;

    QString dataTypeName() const override;

    vtkFloatArray * dataArray();

protected:
    QVtkTableModel * createTableModel() override;

private:
    vtkSmartPointer<vtkFloatArray> m_dataArray;
};
