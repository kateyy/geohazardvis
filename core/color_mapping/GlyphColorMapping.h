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
    ~GlyphColorMapping() override;

protected:
    const QList<GlyphMappingData *> & glyphMappingData() const;

    void assignToVisualization() override;
    void unassignFromVisualization() override;

private:
    QList<GlyphMappingData *> m_glyphMappingData;

private:
    Q_DISABLE_COPY(GlyphColorMapping)
};
