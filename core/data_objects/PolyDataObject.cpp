#include "PolyDataObject.h"

#include <cassert>

#include "core/Input.h"


namespace
{
    QString s_dataTypeName = "polygonal mesh";
}

PolyDataObject::PolyDataObject(std::shared_ptr<PolyDataInput> input)
    : DataObject(input)
{
}

QString PolyDataObject::dataTypeName() const
{
    return s_dataTypeName;
}

std::shared_ptr<PolyDataInput> PolyDataObject::polyDataInput()
{
    assert(std::dynamic_pointer_cast<PolyDataInput>(input()));

    return std::static_pointer_cast<PolyDataInput>(input());
}

std::shared_ptr<const PolyDataInput> PolyDataObject::polyDataInput() const
{
    assert(std::dynamic_pointer_cast<const PolyDataInput>(input()));

    return std::static_pointer_cast<const PolyDataInput>(input());
}
