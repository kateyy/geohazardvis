#pragma once


template<typename SubClass>
QList<GlyphMappingData *> GlyphMappingData::newInstance(RenderedData * renderedData)
{
    GlyphMappingData * mapping = new SubClass(renderedData);
    if (mapping->isValid())
        mapping->initialize();

    return{ mapping };
}
