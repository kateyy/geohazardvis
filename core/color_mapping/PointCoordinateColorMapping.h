#pragma once

#include <core/color_mapping/ColorMappingData.h>


class CORE_API PointCoordinateColorMapping : public ColorMappingData
{
public:
    explicit PointCoordinateColorMapping(const std::vector<AbstractVisualizedData*> & visualizedData);
    ~PointCoordinateColorMapping() override;

    QString name() const override;

    QString scalarsName(AbstractVisualizedData & vis) const override;
    IndexType scalarsAssociation(AbstractVisualizedData & vis) const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData & visualizedData, int connection = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper, int connection = 0) override;

protected:
    static std::vector<std::unique_ptr<ColorMappingData>> newInstances(const std::vector<AbstractVisualizedData*> & visualizedData);

    std::vector<ValueRange<>> updateBounds() override;

private:
    static const bool s_isRegistered;

private:
    Q_DISABLE_COPY(PointCoordinateColorMapping)
};
