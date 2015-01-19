#pragma once

#include <QMap>

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

    vtkAlgorithm * createFilter(AbstractVisualizedData * visualizedData) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper) override;

protected:
    static QList<ColorMappingData *> newInstances(const QList<AbstractVisualizedData*> & visualizedData);

    void updateBounds() override;

private:
    static const bool s_isRegistered;

    const int m_attributeLocation;
    const QString m_dataArrayName;

    QMap<AbstractVisualizedData *, vtkSmartPointer<vtkVectorNorm>> m_vectorNorms;
};