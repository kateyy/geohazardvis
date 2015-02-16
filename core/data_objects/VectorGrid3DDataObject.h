#pragma once

#include <core/data_objects/DataObject.h>


class vtkImageData;
class vtkAssignAttribute;


class CORE_API VectorGrid3DDataObject : public DataObject
{
public:
    VectorGrid3DDataObject(QString name, vtkImageData * dataSet);
    ~VectorGrid3DDataObject() override;

    bool is3D() const override;

    /** create a rendered instance */
    RenderedData * createRendered() override;

    void addDataArray(vtkDataArray * dataArray) override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

    /** number of values on each axis (x, y, z) */
    const int * dimensions();
    /** index of first and last point on each axis (min/max per x, y, z) */
    const int * extent();
    /** number of vector data components  */
    int numberOfComponents();
    /** scalar range for specified vector component */
    const double * scalarRange(int component);

protected:
    QVtkTableModel * createTableModel() override;
};
