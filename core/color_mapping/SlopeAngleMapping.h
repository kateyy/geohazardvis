#pragma once

#include <core/color_mapping/ColorMappingData.h>


class CORE_API SlopeAngleMapping : public ColorMappingData
{
public:
    SlopeAngleMapping(const QList<AbstractVisualizedData *> & visualizedData);

    QString name() const override;
    QString scalarsName() const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData * visualizedData, int connection = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * mapper) override;

protected:
    static std::vector<std::unique_ptr<ColorMappingData>> newInstance(const QList<AbstractVisualizedData *> & visualizedData);

    QMap<int, QPair<double, double>> updateBounds() override;

private:
    static const bool s_isRegistered;

    QMap<AbstractVisualizedData *, QMap<int, vtkSmartPointer<vtkAlgorithm>>> m_filters;
};
