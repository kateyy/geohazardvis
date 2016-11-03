#pragma once

#include <vector>

#include <QMap>

#include <core/color_mapping/GlyphColorMapping.h>


class vtkAlgorithm;
class vtkVectorNorm;

class GlyphMappingData;


class CORE_API GlyphMagnitudeColorMapping : public GlyphColorMapping
{
public:
    GlyphMagnitudeColorMapping(const QList<AbstractVisualizedData *> & visualizedData,
        const QList<GlyphMappingData *> & glyphMappingData,
        const QString & vectorsName);
    ~GlyphMagnitudeColorMapping() override;

    QString name() const override;
    QString scalarsName(AbstractVisualizedData & vis) const override;
    IndexType scalarsAssociation(AbstractVisualizedData & vis) const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData & visualizedData, int connection = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper) override;

protected:
    static std::vector<std::unique_ptr<ColorMappingData>> newInstances(const QList<AbstractVisualizedData*> & visualizedData);

    std::vector<ValueRange<>> updateBounds() override;

private:
    static const bool s_isRegistered;

    const QString m_vectorName;
    QMap<AbstractVisualizedData *, std::vector<vtkSmartPointer<vtkVectorNorm>>> m_vectorNorms;
    QMap<AbstractVisualizedData *, std::vector<vtkSmartPointer<vtkAlgorithm>>> m_assignedVectors;

private:
    Q_DISABLE_COPY(GlyphMagnitudeColorMapping)
};
