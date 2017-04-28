#pragma once

#include <core/glyph_mapping/GlyphMappingData.h>


class vtkAssignAttribute;


class CORE_API PointCloudDataAttributeGlyphMapping : public GlyphMappingData
{
public:
    PointCloudDataAttributeGlyphMapping(
        RenderedData & renderedData,
        const QString & attributeName);

    QString name() const override;

    IndexType scalarsAssociation() const override;

    vtkAlgorithmOutput * vectorDataOutputPort() override;

protected:
    static std::vector<std::unique_ptr<GlyphMappingData>> newInstances(RenderedData & renderedData);

private:
    static const bool s_registered;

    vtkSmartPointer<vtkAssignAttribute> m_assignVectors;
    QString m_attributeName;
};
