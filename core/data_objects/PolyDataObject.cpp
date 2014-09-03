#include "PolyDataObject.h"

#include <cassert>

#include "core/Input.h"
#include <core/data_objects/RenderedPolyData.h>


namespace
{
    QString s_dataTypeName = "polygonal mesh";
}

PolyDataObject::PolyDataObject(std::shared_ptr<PolyDataInput> input)
    : DataObject(input)
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
