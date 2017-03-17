#pragma once

#include <array>

#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <core/utility/DataExtent_fwd.h>


class vtkImageData;
class vtkAssignAttribute;


class CORE_API VectorGrid3DDataObject : public CoordinateTransformableDataObject
{
public:
    VectorGrid3DDataObject(const QString & name, vtkImageData & dataSet);
    ~VectorGrid3DDataObject() override;

    std::unique_ptr<DataObject> newInstance(const QString & name, vtkDataSet * dataSet) const override;

    bool is3D() const override;
    IndexType defaultAttributeLocation() const override;

    std::unique_ptr<RenderedData> createRendered() override;

    void addDataArray(vtkDataArray & dataArray) override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

    vtkImageData & imageData();
    const vtkImageData & imageData() const;

    /** index of first and last point on each axis (min/max per x, y, z) */
    ImageExtent extent();
    /** number of vector data components  */
    int numberOfComponents();
    /** scalar range for specified vector component */
    ValueRange<> scalarRange(int component);

protected:
    std::unique_ptr<QVtkTableModel> createTableModel() override;

    bool checkIfStructureChanged() override;

private:
    std::array<int, 6> m_extent;

private:
    Q_DISABLE_COPY(VectorGrid3DDataObject)
};
