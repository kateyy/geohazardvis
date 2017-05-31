#include "GlyphMagnitudeColorMapping.h"

#include <cassert>

#include <vtkAssignAttribute.h>
#include <vtkDataSet.h>
#include <vtkLookupTable.h>
#include <vtkMapper.h>
#include <vtkPassThrough.h>
#include <vtkPointData.h>
#include <vtkVectorNorm.h>

#include <core/types.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/filters/ArrayChangeInformationFilter.h>
#include <core/rendered_data/RenderedData3D.h>
#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>
#include <core/utility/DataExtent.h>
#include <core/utility/macros.h>


namespace
{
    const QString s_name = "Glyph Magnitude";

    const char * const s_normScalarsName = "VectorNorm";
}

const bool GlyphMagnitudeColorMapping::s_isRegistered =
    ColorMappingRegistry::instance().registerImplementation(s_name, newInstances);


std::vector<std::unique_ptr<ColorMappingData>> GlyphMagnitudeColorMapping::newInstances(const std::vector<AbstractVisualizedData*> & visualizedData)
{
    // If multiple visualizations share glyph mappings with the same name, the same glyph color
    // mapping will be created for them. Note that this is only relevant if these visualizations
    // share a color mapping instance.
    std::map<QString, std::vector<std::pair<RenderedData3D *, GlyphMappingData *>>> glyphMappingDataByName;

    for (auto vis : visualizedData)
    {
        auto renderedData3D = dynamic_cast<RenderedData3D *>(vis);
        if (!renderedData3D)
        {
            continue;
        }

        for (auto glyphMappingData : renderedData3D->glyphMapping().vectors())
        {
            if (glyphMappingData->isVisible())
            {
                glyphMappingDataByName[glyphMappingData->name()].push_back(
                    std::make_pair(renderedData3D, glyphMappingData));
            }
        }
    }

    std::vector<std::unique_ptr<ColorMappingData>> instances;
    for (auto && it : glyphMappingDataByName)
    {
        instances.push_back(std::make_unique<GlyphMagnitudeColorMapping>(
            visualizedData,
            it.first,
            std::map<RenderedData3D *, GlyphMappingData *>(it.second.begin(), it.second.end())));
    }

    return instances;
}

GlyphMagnitudeColorMapping::GlyphMagnitudeColorMapping(
    const std::vector<AbstractVisualizedData *> & visualizedData,
    const QString & vectorName,
    const std::map<RenderedData3D *, GlyphMappingData *> & glyphMappingData)
    : GlyphColorMapping(visualizedData, glyphMappingData)
    , m_vectorName{ vectorName }
{
    m_isValid = !glyphMappingData.empty();
}

GlyphMagnitudeColorMapping::~GlyphMagnitudeColorMapping() = default;

QString GlyphMagnitudeColorMapping::name() const
{
    return "Glyph: " + m_vectorName + " Magnitude";
}

QString GlyphMagnitudeColorMapping::scalarsName(AbstractVisualizedData & /*vis*/) const
{
    return m_vectorName;
}

vtkSmartPointer<vtkAlgorithm> GlyphMagnitudeColorMapping::createFilter(AbstractVisualizedData & visualizedData, unsigned int port)
{
    if (port != 0)
    {
        return vtkSmartPointer<vtkPassThrough>::New();
    }

    const auto filterIt = m_filters.find(&visualizedData);

    // Return already created filter (in this case, visualizedData is valid for this mapping)
    if (filterIt != m_filters.end())
    {
        return filterIt->second;
    }

    // Check if the mapping can be applied to the provided visualization
    const auto glyphMappingDataIt = m_glyphMappingData.find(static_cast<RenderedData3D *>(&visualizedData));
    if (glyphMappingDataIt == m_glyphMappingData.end())
    {
        return vtkSmartPointer<vtkPassThrough>::New();
    }

    // Create the mapping pipeline
    assert(glyphMappingDataIt->second);
    auto & glyphMappingData = *glyphMappingDataIt->second;

    auto norm = vtkSmartPointer<vtkVectorNorm>::New();
    norm->SetInputConnection(glyphMappingData.vectorDataOutputPort());

    auto assignVectors = vtkSmartPointer<vtkAssignAttribute>::New();
    assignVectors->Assign(m_vectorName.toUtf8().data(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);
    assignVectors->SetInputConnection(norm->GetOutputPort());

    auto setArrayName = vtkSmartPointer<ArrayChangeInformationFilter>::New();
    setArrayName->EnableRenameOn();
    setArrayName->SetArrayName(s_normScalarsName);
    setArrayName->SetAttributeType(vtkDataSetAttributes::SCALARS);
    setArrayName->SetAttributeLocation(IndexType::points);
    setArrayName->SetInputConnection(assignVectors->GetOutputPort());

    m_filters[&visualizedData] = setArrayName;

    return setArrayName;
}

bool GlyphMagnitudeColorMapping::usesFilter() const
{
    return true;
}

void GlyphMagnitudeColorMapping::configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper, unsigned int port)
{
    GlyphColorMapping::configureMapper(visualizedData, mapper, port);

    auto filterIt = m_filters.find(&visualizedData);
    if (filterIt == m_filters.end())
    {
        return;
    }

    if (auto m = vtkMapper::SafeDownCast(&mapper))
    {
        m->UseLookupTableScalarRangeOn();
        m->SetLookupTable(m_lut);
        m->ScalarVisibilityOn();
        m->SetColorModeToMapScalars();
        m->SetScalarModeToUsePointFieldData();
        m->SelectColorArray(s_normScalarsName);
    }
}

std::vector<ValueRange<>> GlyphMagnitudeColorMapping::updateBounds()
{
    decltype(updateBounds())::value_type totalRange;

    for (auto & mapping : m_glyphMappingData)
    {
        // access lazily created filters
        auto && filter = createFilter(*mapping.first);
        assert(filter && filter->GetNumberOfInputConnections(0) > 0);
        filter->Update();

        // At this pipeline stage, initial cell attributes are associated with the intermediate
        // point geometry.
        auto dataSet = vtkDataSet::SafeDownCast(filter->GetOutputDataObject(0));
        assert(dataSet);
        auto normData = dataSet->GetPointData()->GetScalars();
        assert(normData);

        decltype(totalRange) range;
        normData->GetRange(range.data());
        totalRange.add(range);
    }

    return{ totalRange };
}
