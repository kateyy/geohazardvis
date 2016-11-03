#pragma once

#include <map>
#include <vector>

#include <core/color_mapping/ColorMappingData.h>


class CORE_API CentroidColorMapping : public ColorMappingData
{
public:
    CentroidColorMapping(const QList<AbstractVisualizedData*> & visualizedData);
    ~CentroidColorMapping() override;

    QString name() const override;

    QString scalarsName(AbstractVisualizedData & vis) const override;
    IndexType scalarsAssociation(AbstractVisualizedData & vis) const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData & visualizedData, int connection = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper, int connection = 0) override;

protected:
    static std::vector<std::unique_ptr<ColorMappingData>> newInstances(const QList<AbstractVisualizedData*> & visualizedData);

    std::vector<ValueRange<>> updateBounds() override;

private:
    static const bool s_isRegistered;
    std::map<AbstractVisualizedData *, std::vector<vtkSmartPointer<vtkAlgorithm>>> m_filters;

private:
    Q_DISABLE_COPY(CentroidColorMapping)
};
