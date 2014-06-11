#pragma once

#include <core/data_objects/DataObject.h>
#include <core/core_api.h>


class GridDataInput;


class CORE_API ImageDataObject : public DataObject
{
public:
    ImageDataObject(std::shared_ptr<GridDataInput> input);

    std::shared_ptr<GridDataInput> gridDataInput();
    std::shared_ptr<const GridDataInput> gridDataInput() const;
};
