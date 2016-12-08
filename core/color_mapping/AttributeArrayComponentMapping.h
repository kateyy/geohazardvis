#pragma once

#include <map>

#include <core/color_mapping/ColorMappingData.h>


class CORE_API AttributeArrayComponentMapping : public ColorMappingData
{
public:
    AttributeArrayComponentMapping(const std::vector<AbstractVisualizedData *> & visualizedData,
        const QString & dataArrayName, int numDataComponents, const std::map<AbstractVisualizedData *, IndexType> & attributeLocations);
    ~AttributeArrayComponentMapping() override;

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

    /** name of the attribute array that will be mapped to colors */
    const QString m_dataArrayName;
    /** Per data set, the location where to find the data array, named as above.
     * Assumption: for a specific data set type, a specific attribute location is generally used
     * e.g.: point data for images, cell data for polygonal data */
    const std::map<AbstractVisualizedData *, IndexType> m_attributeLocations;
};
