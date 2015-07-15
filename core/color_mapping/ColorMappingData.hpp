#pragma once

#include "ColorMappingData.h"

template<typename SubClass>
std::vector<std::unique_ptr<ColorMappingData>> ColorMappingData::newInstance(const QList<AbstractVisualizedData*> & visualizedData)
{
    auto mapping = std::make_unique<SubClass>(visualizedData);
    if (mapping->isValid())
        mapping->initialize();
    else
        return{};

    std::vector<std::unique_ptr<ColorMappingData>> result;
    result.push_back(std::move(mapping));
    return result;
}
