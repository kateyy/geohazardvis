#pragma once

#include <core/data_objects/DataObject.h>
#include <core/core_api.h>


class vtkImageData;


class CORE_API ImageDataObject : public DataObject
{
public:
    ImageDataObject(QString name, vtkImageData * dataSet);

    bool is3D() const override;

    RenderedData * createRendered() override;

    QString dataTypeName() const override;

    vtkImageData * imageData();
    const vtkImageData * imageData() const;

    /** number of values on each axis (x, y, z) */
    const int * dimensions();
    /** index of first and last point on each axis (min/max per x, y, z) */
    const int * extent();
    /** scalar range */
    const double * minMaxValue();
};
