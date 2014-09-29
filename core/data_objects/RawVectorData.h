#pragma once

#include <core/data_objects/DataObject.h>


class vtkFloatArray;


class CORE_API RawVectorData : public DataObject
{
public:
    RawVectorData(QString name, vtkFloatArray * dataArray);
    ~RawVectorData() override;

    bool is3D() const override;

    RenderedData * createRendered() override;

    QString dataTypeName() const override;

    vtkFloatArray * dataArray();

protected:
    QVtkTableModel * createTableModel() override;

private:
    vtkSmartPointer<vtkFloatArray> m_dataArray;
};
