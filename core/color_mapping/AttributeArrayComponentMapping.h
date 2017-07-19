/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
        const std::vector<QString> & componentNames,
        const std::map<AbstractVisualizedData *, IndexType> & attributeLocations);
    ~AttributeArrayComponentMapping() override;

    QString name() const override;
    QString scalarsName(AbstractVisualizedData & vis) const override;
    IndexType scalarsAssociation(AbstractVisualizedData & vis) const override;
    bool isTemporalAttribute() const override;

    QString componentName(int component) const override;

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
    const std::vector<QString> m_componentNames;
    /** Per data set, the location where to find the data array, named as above.
     * Assumption: for a specific data set type, a specific attribute location is generally used
     * e.g.: point data for images, cell data for polygonal data */
    const std::map<AbstractVisualizedData *, IndexType> m_attributeLocations;
    std::map<AbstractVisualizedData *, std::map<unsigned int, vtkSmartPointer<vtkAlgorithm>>> m_filters;
};
