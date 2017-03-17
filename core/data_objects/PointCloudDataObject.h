#pragma once

#include <vtkSmartPointer.h>

#include <core/data_objects/GenericPolyDataObject.h>


class CORE_API PointCloudDataObject : public GenericPolyDataObject
{
public:
    PointCloudDataObject(const QString & name, vtkPolyData & dataSet);
    ~PointCloudDataObject() override;

    std::unique_ptr<DataObject> newInstance(const QString & name, vtkDataSet * dataSet) const override;

    IndexType defaultAttributeLocation() const override;

    void addDataArray(vtkDataArray & dataArray) override;

    std::unique_ptr<RenderedData> createRendered() override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

protected:
    std::unique_ptr<QVtkTableModel> createTableModel() override;

private:
    Q_DISABLE_COPY(PointCloudDataObject)
};
