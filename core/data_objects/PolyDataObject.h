#pragma once

#include "DataObject.h"


class PolyDataInput;


class PolyDataObject : public DataObject
{
public:
    PolyDataObject(std::shared_ptr<PolyDataInput> input);

    std::shared_ptr<PolyDataInput> polyDataInput();
    std::shared_ptr<const PolyDataInput> polyDataInput() const;
};
