#pragma once

#include <core/color_mapping/ColorMappingData.h>


class GlyphMappingData;
class RenderedData3D;


class CORE_API GlyphColorMapping : public ColorMappingData
{
public:
    GlyphColorMapping(const QList<AbstractVisualizedData *> & visualizedData,
        const QList<GlyphMappingData *> & glyphMappingData,
        int numDataComponents = 1);

    /** override superclass activate/deactivate: enable color mapping on glyph representations */
    void activate() override;
    void deactivate() override;

protected:
    const QList<GlyphMappingData *> & glyphMappingData() const;

private:
    QList<GlyphMappingData *> m_glyphMappingData;
};
