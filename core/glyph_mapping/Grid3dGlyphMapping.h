#pragma once

#include <vtkSmartPointer.h>

#include <core/glyph_mapping/GlyphMappingData.h>


class vtkAssignAttribute;
class vtkDataArray;
class RenderedVectorGrid3D;


class CORE_API Grid3dGlyphMapping : public GlyphMappingData
{
public:
    QString name() const override;

    vtkAlgorithmOutput * vectorDataOutputPort() override;

protected:
    /** create an instance for each 3D vector array found in the renderedData */
    static QList<GlyphMappingData *> newInstances(RenderedData * renderedData);

    Grid3dGlyphMapping(RenderedVectorGrid3D * renderedGrid, vtkDataArray * dataArray);

protected:
    void updateArrowLength();

private:
    static const bool s_registered;

    vtkSmartPointer<vtkAssignAttribute> m_assignVectors;
    RenderedVectorGrid3D * m_renderedGrid;
    vtkDataArray * m_dataArray;
};
