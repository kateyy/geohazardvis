#pragma once

#include <core/data_objects/DataObject.h>
#include <core/core_api.h>


class PolyDataInput;


class CORE_API PolyDataObject : public DataObject
{
public:
    PolyDataObject(std::shared_ptr<PolyDataInput> input);

    RenderedData * createRendered() override;

    QString dataTypeName() const override;
};
