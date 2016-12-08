#include "GlyphColorMappingGlyphListener.h"

#include <core/rendered_data/RenderedData3D.h>
#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>
#include <core/utility/qthelper.h>


GlyphColorMappingGlyphListener::GlyphColorMappingGlyphListener(QObject * parent)
    : QObject(parent)
{
}

GlyphColorMappingGlyphListener::~GlyphColorMappingGlyphListener() = default;

void GlyphColorMappingGlyphListener::setData(const std::vector<AbstractVisualizedData *> & visualizedData)
{
    m_data.clear();

    disconnectAll(m_connects);

    for (auto vis : visualizedData)
    {
        auto rendered3D = dynamic_cast<RenderedData3D *>(vis);
        if (!rendered3D)
        {
            continue;
        }

        m_data.emplace_back(rendered3D);

        auto & glyphMapping = rendered3D->glyphMapping();
        m_connects.emplace_back(connect(&glyphMapping, &GlyphMapping::vectorsChanged,
            this, &GlyphColorMappingGlyphListener::glyphMappingChanged));

        for (auto data : glyphMapping.vectors())
        {
            m_connects.emplace_back(connect(data, &GlyphMappingData::visibilityChanged,
                this, &GlyphColorMappingGlyphListener::glyphMappingChanged));
        }
    }
}
