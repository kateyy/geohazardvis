#pragma once


template<typename SubClass>
QList<VectorMappingData *> VectorMappingData::newInstance(RenderedData * renderedData)
{
    VectorMappingData * mapping = new SubClass(renderedData);
    if (mapping->isValid())
        mapping->initialize();

    return{ mapping };
}
