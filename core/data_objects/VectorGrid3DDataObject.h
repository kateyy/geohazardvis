#pragma once

#include <core/data_objects/DataObject.h>


class vtkImageData;


class VectorGrid3DDataObject : public DataObject
{
public:
    VectorGrid3DDataObject(QString name, vtkImageData * dataSet);
    ~VectorGrid3DDataObject() override;

    bool is3D() const override;

    /** create a rendered instance */
    RenderedData * createRendered() override;

    QString dataTypeName() const override;

protected:
    QVtkTableModel * createTableModel() override;
};
