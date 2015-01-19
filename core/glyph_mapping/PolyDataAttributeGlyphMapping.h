#pragma once

#include <vtkSmartPointer.h>

#include <core/glyph_mapping/GlyphMappingData.h>


class vtkDataArray;

class PolyDataObject;


class CORE_API PolyDataAttributeGlyphMapping : public GlyphMappingData
{
public:
    ~PolyDataAttributeGlyphMapping() override;

    QString name() const override;

protected:
    /** create an instance for each 3D vector array found in the renderedData */
    static QList<GlyphMappingData *> newInstances(RenderedData * renderedData);

    /** create an instances that maps vectors from vectorData to the renderedData's geometry */
    PolyDataAttributeGlyphMapping(RenderedData * renderedData, vtkDataArray * vectorData);

    void initialize() override;

private:
    static const bool s_registered;

    PolyDataObject * m_polyData;
    vtkSmartPointer<vtkDataArray> m_dataArray;
};
