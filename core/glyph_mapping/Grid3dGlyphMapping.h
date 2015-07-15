#pragma once

#include <core/glyph_mapping/GlyphMappingData.h>


class vtkAssignAttribute;
class vtkDataArray;
class RenderedVectorGrid3D;


class CORE_API Grid3dGlyphMapping : public GlyphMappingData
{
public:
    Grid3dGlyphMapping(RenderedVectorGrid3D & renderedGrid, vtkDataArray * dataArray);

    QString name() const override;

    vtkAlgorithmOutput * vectorDataOutputPort() override;

protected:
    /** create an instance for each 3D vector array found in the renderedData */
    static std::vector<std::unique_ptr<GlyphMappingData>> newInstances(RenderedData & renderedData);

protected:
    void updateArrowLength();

private:
    static const bool s_registered;

    vtkSmartPointer<vtkAssignAttribute> m_assignVectors;
    RenderedVectorGrid3D & m_renderedGrid;
    vtkDataArray * m_dataArray;
};
