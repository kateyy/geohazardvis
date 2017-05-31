#pragma once

#include <map>

#include <core/color_mapping/ColorMappingData.h>


class CORE_API SlopeAngleMapping : public ColorMappingData
{
public:
    explicit SlopeAngleMapping(const std::vector<AbstractVisualizedData *> & visualizedData);
    ~SlopeAngleMapping() override;

    QString name() const override;
    QString scalarsName(AbstractVisualizedData & vis) const override;
    IndexType scalarsAssociation(AbstractVisualizedData & vis) const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData & visualizedData, unsigned int port = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper, unsigned int port = 0) override;

protected:
    static std::vector<std::unique_ptr<ColorMappingData>> newInstance(const std::vector<AbstractVisualizedData *> & visualizedData);

    std::vector<ValueRange<>> updateBounds() override;

private:
    static const bool s_isRegistered;

    std::map<AbstractVisualizedData *, std::map<unsigned int, vtkSmartPointer<vtkAlgorithm>>> m_filters;

private:
    Q_DISABLE_COPY(SlopeAngleMapping)
};
