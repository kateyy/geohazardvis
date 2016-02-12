#pragma once

#include <array>

#include <core/data_objects/DataObject.h>


class vtkImageData;


class CORE_API ImageDataObject : public DataObject
{
public:
    explicit ImageDataObject(const QString & name, vtkImageData & dataSet);
    ~ImageDataObject() override;

    bool is3D() const override;

    std::unique_ptr<RenderedData> createRendered() override;

    void addDataArray(vtkDataArray & dataArray) override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

    vtkImageData * imageData();
    const vtkImageData * imageData() const;

    /** @return scalars assigned to the image point data. All ImageDataObjects are assumed to have valid scalar data
        with a name set. */
    vtkDataArray & scalars();

    /** number of values on each axis (x, y, z) */
    const int * dimensions();
    /** index of first and last point on each axis (min/max per x, y, z) */
    const int * extent();
    /** scalar range */
    const double * minMaxValue();

protected:
    std::unique_ptr<QVtkTableModel> createTableModel() override;

    bool checkIfStructureChanged() override;

private:
    std::array<int, 6> m_extent;
};
