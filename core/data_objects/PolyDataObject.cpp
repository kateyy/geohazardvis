#include "PolyDataObject.h"

#include <cassert>

#include <vtkPolyData.h>

#include <core/data_objects/RenderedPolyData.h>


namespace
{
    QString s_dataTypeName = "polygonal mesh";
}

PolyDataObject::PolyDataObject(QString name, vtkPolyData * dataSet)
    : DataObject(name, dataSet)
{
}

RenderedData * PolyDataObject::createRendered()
{
    return new RenderedPolyData(this);
}

QString PolyDataObject::dataTypeName() const
{
    return s_dataTypeName;
}
