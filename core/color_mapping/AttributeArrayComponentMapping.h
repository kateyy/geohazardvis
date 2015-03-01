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

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData * visualizedData, int connection = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * mapper) override;

protected:
    static QList<ColorMappingData *> newInstances(const QList<AbstractVisualizedData *> & visualizedData);

    QMap<vtkIdType, QPair<double, double>> updateBounds() override;

private:
    static const bool s_isRegistered;

    const int m_attributeLocation;
    const QString m_dataArrayName;
};
