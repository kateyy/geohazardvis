#pragma once

#include "ColorMappingData.h"

template<typename SubClass>
QList<ColorMappingData *> ColorMappingData::newInstance(const QList<AbstractVisualizedData*> & visualizedData)
{
    ColorMappingData * mapping = new SubClass(visualizedData);
    if (mapping->isValid())
        mapping->initialize();

    return{ mapping };
}
