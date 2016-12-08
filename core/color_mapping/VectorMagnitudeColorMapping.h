#pragma once

#include <map>

#include <core/color_mapping/ColorMappingData.h>


class CORE_API VectorMagnitudeColorMapping : public ColorMappingData
{
public:
    VectorMagnitudeColorMapping(
        const std::vector<AbstractVisualizedData *> & visualizedData,
        const QString & dataArrayName, IndexType attributeLocation);
    ~VectorMagnitudeColorMapping() override;

    QString name() const override;
    QString scalarsName(AbstractVisualizedData & vis) const override;
    IndexType scalarsAssociation(AbstractVisualizedData & vis) const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData & visualizedData, unsigned int port = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper, unsigned int port) override;

protected:
    static std::vector<std::unique_ptr<ColorMappingData>> newInstances(const std::vector<AbstractVisualizedData*> & visualizedData);

    std::vector<ValueRange<>> updateBounds() override;

private:
    static const bool s_isRegistered;

    const IndexType m_attributeLocation;
    const QString m_dataArrayName;
    const QString m_magnitudeArrayName;

    std::map<AbstractVisualizedData *, std::vector<vtkSmartPointer<vtkAlgorithm>>> m_filters;

private:
    Q_DISABLE_COPY(VectorMagnitudeColorMapping)
};
