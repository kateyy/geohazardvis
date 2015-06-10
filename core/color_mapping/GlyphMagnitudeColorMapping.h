#pragma once

#include <QMap>

#include <core/color_mapping/GlyphColorMapping.h>


template<typename T> class QVector;
class vtkAssignAttribute;
class vtkVectorNorm;

class GlyphMappingData;


class CORE_API GlyphMagnitudeColorMapping : public GlyphColorMapping
{
public:
    GlyphMagnitudeColorMapping(const QList<AbstractVisualizedData *> & visualizedData,
        const QList<GlyphMappingData *> & glyphMappingData,
        const QString & vectorsName);

    QString name() const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData * visualizedData, int connection = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * mapper) override;

protected:
    static QList<ColorMappingData *> newInstances(const QList<AbstractVisualizedData*> & visualizedData);

    QMap<int, QPair<double, double>> updateBounds() override;

private:
    static const bool s_isRegistered;

    const QString m_vectorName;
    QMap<AbstractVisualizedData *, QVector<vtkSmartPointer<vtkVectorNorm>>> m_vectorNorms;
    QMap<AbstractVisualizedData *, QVector<vtkSmartPointer<vtkAssignAttribute>>> m_assignedVectors;
};
