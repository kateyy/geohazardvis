#pragma once

#include "ScalarsForColorMapping.h"

template<typename SubClass>
QList<ScalarsForColorMapping *> ScalarsForColorMapping::newInstance(const QList<AbstractVisualizedData*> & visualizedData)
{
    ScalarsForColorMapping * mapping = new SubClass(visualizedData);
    if (mapping->isValid())
        mapping->initialize();

    return{ mapping };
}
