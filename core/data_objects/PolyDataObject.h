#pragma once

#include <core/data_objects/DataObject.h>
#include <core/core_api.h>


class vtkPolyData;


class CORE_API PolyDataObject : public DataObject
{
public:
    PolyDataObject(QString name, vtkPolyData * dataSet);

    RenderedData * createRendered() override;

    QString dataTypeName() const override;
};
