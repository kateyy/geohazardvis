#pragma once

#include <core/color_mapping/ColorMappingData.h>


class CORE_API DirectImageColors : public ColorMappingData
{
public:
    DirectImageColors(const std::vector<AbstractVisualizedData *> & visualizedData,
        const QString & dataArrayName, IndexType attributeLocation);
    ~DirectImageColors() override;

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

    const IndexType m_attributeLocation;
    const QString m_dataArrayName;

private:
    Q_DISABLE_COPY(DirectImageColors)
};
