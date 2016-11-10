#include "GlyphColorMapping.h"

#include <cassert>

#include <vtkLookupTable.h>

#include <core/types.h>
#include <core/glyph_mapping/GlyphMappingData.h>
#include <core/rendered_data/RenderedData3D.h>


GlyphColorMapping::GlyphColorMapping(
    const QList<AbstractVisualizedData *> & visualizedData,
    const std::map<RenderedData3D *, GlyphMappingData *> & glyphMappingData,
    int numDataComponents)
    : ColorMappingData(visualizedData, numDataComponents)
    , m_glyphMappingData{ glyphMappingData }
{
}

GlyphColorMapping::~GlyphColorMapping() = default;

IndexType GlyphColorMapping::scalarsAssociation(AbstractVisualizedData & vis) const
{
    auto it = m_glyphMappingData.find(static_cast<RenderedData3D *>(&vis));
    if (it != m_glyphMappingData.end() && it->second)
    {
        assert(it->second);
        return it->second->scalarsAssociation();
    }

    return IndexType::invalid;
}

void GlyphColorMapping::assignToVisualization()
{
    for (auto pair : m_glyphMappingData)
    {
        assert(pair.second);
        pair.second->setColorMappingData(this);
        pair.second->setColorMappingGradient(m_lut);
    }
}

void GlyphColorMapping::unassignFromVisualization()
{
    for (auto pair : m_glyphMappingData)
    {
        assert(pair.second);
        pair.second->setColorMappingData(nullptr);
        pair.second->setColorMappingGradient(nullptr);
    }
}
