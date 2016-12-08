#pragma once

#include <map>

#include <core/color_mapping/ColorMappingData.h>


class GlyphMappingData;
class RenderedData3D;


class CORE_API GlyphColorMapping : public ColorMappingData
{
public:
    /** Construct a color mapping on glyphs.
      * @param visualizedData Required for superclass API, pass-through current visualizations
      * @param glyphMappingData Active glyph mapping per RenderedData3D instance for that color
      * mapping is provided by this GlyphColorMapping instance. Currently, only one glyph color
      * mapping per RenderedData3D can be active at a time (restricted by ColorMappingData API).
      * @param numDataComponents Number of data components of the attribute (superclass API)
     */
    GlyphColorMapping(
        const std::vector<AbstractVisualizedData *> & visualizedData,
        const std::map<RenderedData3D *, GlyphMappingData *> & glyphMappingData,
        int numDataComponents = 1);
    ~GlyphColorMapping() override;

    /** Location of the source data attribute (points, cells, ...).
      * This is the association of the underlying attribute. The glyphs themselves may have a
      * different association (e.g., cell centroids, which are points). */
    IndexType scalarsAssociation(AbstractVisualizedData & vis) const override;

protected:
    void assignToVisualization() override;
    void unassignFromVisualization() override;

    const std::map<RenderedData3D *, GlyphMappingData *> m_glyphMappingData;

private:
    Q_DISABLE_COPY(GlyphColorMapping)
};
