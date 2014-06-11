#pragma once

#include <core/data_objects/DataObject.h>
#include <core/core_api.h>


class PolyDataInput;


class CORE_API PolyDataObject : public DataObject
{
public:
    PolyDataObject(std::shared_ptr<PolyDataInput> input);

    std::shared_ptr<PolyDataInput> polyDataInput();
    std::shared_ptr<const PolyDataInput> polyDataInput() const;
};
