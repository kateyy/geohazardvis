#pragma once

#include <map>
#include <vector>

#include <core/color_mapping/ColorMappingData.h>


class CORE_API CentroidColorMapping : public ColorMappingData
{
public:
    explicit CentroidColorMapping(const std::vector<AbstractVisualizedData*> & visualizedData);
    ~CentroidColorMapping() override;

    QString name() const override;

    QString scalarsName(AbstractVisualizedData & vis) const override;
    IndexType scalarsAssociation(AbstractVisualizedData & vis) const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData & visualizedData, unsigned int port = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper, unsigned int port = 0) override;

protected:
    static std::vector<std::unique_ptr<ColorMappingData>> newInstances(const std::vector<AbstractVisualizedData *> & visualizedData);

    std::vector<ValueRange<>> updateBounds() override;

private:
    static const bool s_isRegistered;
    std::map<AbstractVisualizedData *, std::vector<vtkSmartPointer<vtkAlgorithm>>> m_filters;

private:
    Q_DISABLE_COPY(CentroidColorMapping)
};
