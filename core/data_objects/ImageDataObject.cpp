#include "ImageDataObject.h"

#include <cassert>

#include "core/Input.h"


ImageDataObject::ImageDataObject(std::shared_ptr<GridDataInput> input)
    : DataObject(input)
{
}

std::shared_ptr<GridDataInput> ImageDataObject::gridDataInput()
{
    assert(dynamic_cast<GridDataInput*>(input().get()));
    return std::static_pointer_cast<GridDataInput>(input());
}

std::shared_ptr<const GridDataInput> ImageDataObject::gridDataInput() const
{
    assert(dynamic_cast<const GridDataInput*>(input().get()));
    return std::static_pointer_cast<const GridDataInput>(input());
}
