#pragma once

#include <QMap>
#include <QVector>

#include <vtkSmartPointer.h>

#include <core/color_mapping/ColorMappingData.h>


class vtkVectorNorm;


class CORE_API VectorMagnitudeColorMapping : public ColorMappingData
{
public:
    VectorMagnitudeColorMapping(
        const QList<AbstractVisualizedData *> & visualizedData,
        const QString & dataArrayName, int attributeLocation);
    ~VectorMagnitudeColorMapping() override;

    QString name() const override;

    vtkAlgorithm * createFilter(AbstractVisualizedData * visualizedData, int connection = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * mapper) override;

protected:
    static QList<ColorMappingData *> newInstances(const QList<AbstractVisualizedData*> & visualizedData);

    QMap<vtkIdType, QPair<double, double>> updateBounds() override;

private:
    static const bool s_isRegistered;

    const int m_attributeLocation;
    const QString m_dataArrayName;

    QMap<AbstractVisualizedData *, QVector<vtkSmartPointer<vtkVectorNorm>>> m_vectorNorms;
};
