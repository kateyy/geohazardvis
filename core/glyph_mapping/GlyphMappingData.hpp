#pragma once

#include "GlyphMappingData.h"


template<typename SubClass>
std::vector<std::unique_ptr<GlyphMappingData>> GlyphMappingData::newInstance(RenderedData & renderedData)
{
    auto mapping = std::make_unique<SubClass>(renderedData);
    if (mapping->isValid())
        mapping->initialize();

    std::vector<std::unique_ptr<GlyphMappingData>> result;
    result.push_back(std::move(mapping));
    return result;
}
