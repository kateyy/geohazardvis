#include "GlyphColorMappingGlyphListener.h"

#include <QDebug>

#include <core/rendered_data/RenderedData3D.h>
#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>


GlyphColorMappingGlyphListener::GlyphColorMappingGlyphListener(QObject * parent)
    : QObject(parent)
{
}

void GlyphColorMappingGlyphListener::setData(const QList<AbstractVisualizedData *> & visualizedData)
{
    m_data.clear();
    
    for (auto c : m_connects)
        disconnect(c);
    m_connects.clear();

    for (auto vis : visualizedData)
    {
        RenderedData3D * rendered3D = dynamic_cast<RenderedData3D *>(vis);
        if (!rendered3D)
            continue;

        m_data << rendered3D;

        auto & glyphMapping = rendered3D->glyphMapping();
        m_connects << connect(&glyphMapping, &GlyphMapping::vectorsChanged,
            this, &GlyphColorMappingGlyphListener::glyphMappingChanged);

        for (GlyphMappingData * data : glyphMapping.vectors().values())
            m_connects << connect(data, &GlyphMappingData::visibilityChanged,
                this, &GlyphColorMappingGlyphListener::glyphMappingChanged);
    }
}
