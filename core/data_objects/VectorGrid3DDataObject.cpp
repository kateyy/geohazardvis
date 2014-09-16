#include "VectorGrid3DDataObject.h"

#include <vtkPolyData.h>

//#include <core/data_objects/RenderedVectorGrid3D.h>


namespace
{
const QString s_dataTypeName = "3D vector grid";
}

VectorGrid3DDataObject::VectorGrid3DDataObject(QString name, vtkPolyData * dataSet)
    : DataObject(name, dataSet)
{
}

VectorGrid3DDataObject::~VectorGrid3DDataObject() = default;

RenderedData * VectorGrid3DDataObject::createRendered()
{
    //return new RenderedVectorGrid3D(this);
    return nullptr;
}

QString VectorGrid3DDataObject::dataTypeName() const
{
    return s_dataTypeName;
}
