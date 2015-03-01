#pragma once

#include <QMap>
#include <QVector>

#include <core/color_mapping/GlyphColorMapping.h>


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

    vtkAlgorithm * createFilter(AbstractVisualizedData * visualizedData, int connection = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * mapper) override;

protected:
    static QList<ColorMappingData *> newInstances(const QList<AbstractVisualizedData*> & visualizedData);

    QMap<vtkIdType, QPair<double, double>> updateBounds() override;

private:
    static const bool s_isRegistered;

    const QString m_vectorName;
    QMap<AbstractVisualizedData *, QVector<vtkSmartPointer<vtkVectorNorm>>> m_vectorNorms;
};
