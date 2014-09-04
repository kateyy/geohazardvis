#pragma once


template<typename SubClass>
VectorsForSurfaceMapping * VectorsForSurfaceMapping::newInstance(RenderedData * renderedData)
{
    VectorsForSurfaceMapping * mapping = new SubClass(renderedData);
    if (mapping->isValid())
        mapping->initialize();

    return mapping;
}
