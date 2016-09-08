#include "GlyphColorMapping.h"

#include <vtkLookupTable.h>

#include <core/rendered_data/RenderedData3D.h>
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

void GlyphColorMapping::activate()
{
    for (GlyphMappingData * glyphMapping : glyphMappingData())
    {
        glyphMapping->setColorMappingData(this);
        glyphMapping->setColorMappingGradient(m_lut);
    }
}

void GlyphColorMapping::deactivate()
{
    for (GlyphMappingData * glyphMapping : glyphMappingData())
    {
        glyphMapping->setColorMappingData(nullptr);
    }
}

const QList<GlyphMappingData *> & GlyphColorMapping::glyphMappingData() const
{
    return m_glyphMappingData;
}
