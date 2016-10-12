#pragma once

#include <core/data_objects/CoordinateTransformableDataObject.h>


class vtkPolyData;


class CORE_API GenericPolyDataObject : public CoordinateTransformableDataObject
{
public:
    GenericPolyDataObject(const QString & name, vtkPolyData & dataSet);
    ~GenericPolyDataObject() override;

    bool is3D() const override;

    vtkPolyData & polyDataSet();
    const vtkPolyData & polyDataSet() const;

    static std::unique_ptr<GenericPolyDataObject> createInstance(const QString & name, vtkPolyData & dataSet);

    double pointCoordinateComponent(vtkIdType pointId, int component, bool * validId = nullptr);
    bool setPointCoordinateComponent(vtkIdType pointId, int component, double value);

private:
    Q_DISABLE_COPY(GenericPolyDataObject)
};
