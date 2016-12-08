#pragma once

#include <map>

#include <core/color_mapping/GlyphColorMapping.h>


class vtkVectorNorm;


class CORE_API GlyphMagnitudeColorMapping : public GlyphColorMapping
{
public:
    GlyphMagnitudeColorMapping(
        const std::vector<AbstractVisualizedData *> & visualizedData,
        const QString & vectorName,
        const std::map<RenderedData3D *, GlyphMappingData *> & glyphMappingData);
    ~GlyphMagnitudeColorMapping() override;

    QString name() const override;
    QString scalarsName(AbstractVisualizedData & vis) const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData & visualizedData, unsigned int port = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper, unsigned int port = 0) override;

protected:
    static std::vector<std::unique_ptr<ColorMappingData>> newInstances(const std::vector<AbstractVisualizedData*> & visualizedData);

    std::vector<ValueRange<>> updateBounds() override;

private:
    static const bool s_isRegistered;

    const QString m_vectorName;
    std::map<AbstractVisualizedData *, vtkSmartPointer<vtkAlgorithm>> m_filters;

private:
    Q_DISABLE_COPY(GlyphMagnitudeColorMapping)
};
