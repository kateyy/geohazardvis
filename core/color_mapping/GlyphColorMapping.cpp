#include "GlyphColorMapping.h"

#include <vtkLookupTable.h>

#include <core/rendered_data/RenderedData3D.h>
#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>


GlyphColorMapping::GlyphColorMapping(const QList<AbstractVisualizedData *> & visualizedData,
    const QList<GlyphMappingData *> & glyphMappingData,
    vtkIdType numDataComponent)
    : ColorMappingData(visualizedData, numDataComponent)
    , m_glyphMappingData(glyphMappingData)
{
}

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
