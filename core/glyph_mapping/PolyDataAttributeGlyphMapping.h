#pragma once

#include <vtkSmartPointer.h>

#include <core/glyph_mapping/GlyphMappingData.h>


class vtkAssignAttribute;
class vtkDataArray;

class PolyDataObject;


class CORE_API PolyDataAttributeGlyphMapping : public GlyphMappingData
{
public:
    QString name() const override;

    vtkAlgorithmOutput * vectorDataOutputPort() override;

protected:
    /** create an instance for each 3D vector array found in the renderedData */
    static QList<GlyphMappingData *> newInstances(RenderedData & renderedData);

    /** create an instances that maps vectors from vectorData to the renderedData's geometry */
    PolyDataAttributeGlyphMapping(RenderedData & renderedData, vtkDataArray * vectorData);

private:
    static const bool s_registered;

    vtkSmartPointer<vtkAssignAttribute> m_assignVectors;
    vtkSmartPointer<vtkDataArray> m_dataArray;
};
