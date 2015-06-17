#pragma once

#include <vtkSmartPointer.h>

#include <core/data_objects/DataObject.h>


class vtkFloatArray;


class CORE_API RawVectorData : public DataObject
{
public:
    RawVectorData(const QString & name, vtkFloatArray * dataArray);

    bool is3D() const override;

    RenderedData * createRendered() override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

    vtkFloatArray * dataArray();

protected:
    QVtkTableModel * createTableModel() override;

private:
    vtkSmartPointer<vtkFloatArray> m_dataArray;
};
