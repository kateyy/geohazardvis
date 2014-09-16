#pragma once

#include <core/data_objects/DataObject.h>


class vtkPolyData;


class VectorGrid3DDataObject : public DataObject
{
public:
    VectorGrid3DDataObject(QString name, vtkPolyData * dataSet);
    ~VectorGrid3DDataObject() override;

    /** create a rendered instance */
    RenderedData * createRendered() override;

    QString dataTypeName() const override;
};
