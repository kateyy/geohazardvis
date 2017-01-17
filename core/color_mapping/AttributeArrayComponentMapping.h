#pragma once

#include <map>

#include <core/color_mapping/ColorMappingData.h>


/**
 * Surface color mapping for data attributes stored in (processed) data sets.
 *
 * This color mapping covers (nearly) all cases for mapping of attributes that are persistently
 * stored in a data set. Attribute mapping is implement for point and cell attributes. Attributes
 * are uniquely identified by their name.
 * For multi-component attributes, one component is selected at a time.
 * In the initial setup, this class checks if vtkDataObject::DATA_TIME_STEP is present in attribute
 * array information to report temporal attributes. Selection of specific time steps has to be done
 * in the upstream pipeline.
*/
class CORE_API AttributeArrayComponentMapping : public ColorMappingData
{
public:
    AttributeArrayComponentMapping(const std::vector<AbstractVisualizedData *> & visualizedData,
        const QString & dataArrayName,
        int numDataComponents,
        bool isTemporalAttribute,
        const std::map<AbstractVisualizedData *, IndexType> & attributeLocations);
    ~AttributeArrayComponentMapping() override;

    QString name() const override;
    QString scalarsName(AbstractVisualizedData & vis) const override;
    IndexType scalarsAssociation(AbstractVisualizedData & vis) const override;
    bool isTemporalAttribute() const override;

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
    const bool m_isTemporalAttribute;
    /** Per data set, the location where to find the data array, named as above.
     * Assumption: for a specific data set type, a specific attribute location is generally used
     * e.g.: point data for images, cell data for polygonal data */
    const std::map<AbstractVisualizedData *, IndexType> m_attributeLocations;
};
