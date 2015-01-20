#pragma once

#include <core/color_mapping/ColorMappingData.h>


class CORE_API AttributeArrayComponentMapping : public ColorMappingData
{
public:
    AttributeArrayComponentMapping(const QList<AbstractVisualizedData *> & visualizedData,
        QString dataArrayName, int attributeLocation, vtkIdType numDataComponents);
    ~AttributeArrayComponentMapping() override;

    QString name() const override;
    QString scalarsName() const override;

    vtkAlgorithm * createFilter(AbstractVisualizedData * visualizedData) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper) override;

protected:
    static QList<ColorMappingData *> newInstances(const QList<AbstractVisualizedData *> & visualizedData);

    QMap<vtkIdType, QPair<double, double>> updateBounds() override;

private:
    static const bool s_isRegistered;

    const int m_attributeLocation;
    const QString m_dataArrayName;
};
