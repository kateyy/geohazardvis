#include "GlyphColorMapping.h"

#include <vtkLookupTable.h>

#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>


GlyphColorMapping::GlyphColorMapping(const QList<AbstractVisualizedData *> & visualizedData,
    const QList<GlyphMappingData *> & glyphMappingData,
    int numDataComponents)
    : ColorMappingData(visualizedData, numDataComponents)
    , m_glyphMappingData(glyphMappingData)
{
}

GlyphColorMapping::~GlyphColorMapping() = default;

const QList<GlyphMappingData *> & GlyphColorMapping::glyphMappingData() const
{
    return m_glyphMappingData;
}

void GlyphColorMapping::assignToVisualization()
{
    for (auto glyphMapping : m_glyphMappingData)
    {
        glyphMapping->setColorMappingData(this);
        glyphMapping->setColorMappingGradient(m_lut);
    }
}

void GlyphColorMapping::unassignFromVisualization()
{
    for (auto glyphMapping : m_glyphMappingData)
    {
        glyphMapping->setColorMappingData(nullptr);
        glyphMapping->setColorMappingGradient(nullptr);
    }
}
