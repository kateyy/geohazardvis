#pragma once

#include <vtkSmartPointer.h>

#include <core/data_objects/DataObject.h>


class vtkFloatArray;


class CORE_API RawVectorData : public DataObject
{
public:
    RawVectorData(const QString & name, vtkFloatArray & dataArray);
    ~RawVectorData() override;

    /** Unsupported for this class, as the ctor requires a data array as 2nd parameter. */
    std::unique_ptr<DataObject> newInstance(const QString & name, vtkDataSet * dataSet) const override;

    bool is3D() const override;
    IndexType defaultAttributeLocation() const override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

    vtkFloatArray * dataArray();

protected:
    std::unique_ptr<QVtkTableModel> createTableModel() override;

private:
    vtkSmartPointer<vtkFloatArray> m_dataArray;

private:
    Q_DISABLE_COPY(RawVectorData)
};
