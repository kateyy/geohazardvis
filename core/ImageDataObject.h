#pragma once

#include "DataObject.h"


class GridDataInput;


class ImageDataObject : public DataObject
{
public:
    ImageDataObject(std::shared_ptr<GridDataInput> input);

    std::shared_ptr<GridDataInput> gridDataInput();
    std::shared_ptr<const GridDataInput> gridDataInput() const;
};
