#pragma once

#include <core/data_objects/DataObject.h>


class vtkImageData;


class CORE_API ImageDataObject : public DataObject
{
public:
    ImageDataObject(const QString & name, vtkImageData & dataSet);

    bool is3D() const override;

    RenderedData * createRendered() override;

    void addDataArray(vtkDataArray & dataArray) override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

    vtkImageData * imageData();
    const vtkImageData * imageData() const;

    /** number of values on each axis (x, y, z) */
    const int * dimensions();
    /** index of first and last point on each axis (min/max per x, y, z) */
    const int * extent();
    /** scalar range */
    const double * minMaxValue();

protected:
    QVtkTableModel * createTableModel() override;
};
