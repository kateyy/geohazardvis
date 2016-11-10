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


std::vector<std::unique_ptr<ColorMappingData>> GlyphMagnitudeColorMapping::newInstances(const QList<AbstractVisualizedData*> & visualizedData)
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
            std::map<RenderedData3D *, GlyphMappingData *>({ it.second.begin(), it.second.end() })));
    }

    return instances;
}

GlyphMagnitudeColorMapping::GlyphMagnitudeColorMapping(
    const QList<AbstractVisualizedData *> & visualizedData,
    const QString & vectorName,
    const std::map<RenderedData3D *, GlyphMappingData *> & glyphMappingData)
    : GlyphColorMapping(visualizedData, glyphMappingData)
    , m_vectorName{ vectorName }
{
    for (auto pair : glyphMappingData)
    {
        auto norm = vtkSmartPointer<vtkVectorNorm>::New();
        norm->SetInputConnection(pair.second->vectorDataOutputPort());

        auto assignVectors = vtkSmartPointer<vtkAssignAttribute>::New();
        assignVectors->Assign(vectorName.toUtf8().data(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);
        assignVectors->SetInputConnection(norm->GetOutputPort());

        auto setArrayName = vtkSmartPointer<ArrayChangeInformationFilter>::New();
        setArrayName->EnableRenameOn();
        setArrayName->SetArrayName(s_normScalarsName);
        setArrayName->SetAttributeType(vtkDataSetAttributes::SCALARS);
        setArrayName->SetAttributeLocation(ArrayChangeInformationFilter::POINT_DATA);
        setArrayName->SetInputConnection(assignVectors->GetOutputPort());

        m_vectorNorms.emplace(pair.first, norm);
        m_assignedVectors.emplace(pair.first, setArrayName);

        m_isValid = true;
    }
}

GlyphMagnitudeColorMapping::~GlyphMagnitudeColorMapping() = default;

QString GlyphMagnitudeColorMapping::name() const
{
    return "Glyph Magnitude: " + m_vectorName;
}

QString GlyphMagnitudeColorMapping::scalarsName(AbstractVisualizedData & /*vis*/) const
{
    return m_vectorName;
}

vtkSmartPointer<vtkAlgorithm> GlyphMagnitudeColorMapping::createFilter(AbstractVisualizedData & visualizedData, int DEBUG_ONLY(connection))
{
    auto && filterIt = m_assignedVectors.find(&visualizedData);

    // Required/valid filters are already created. If no filter was found, color mapping is not
    // implemented for this visualization
    if (filterIt == m_assignedVectors.end())
    {
        return vtkSmartPointer<vtkPassThrough>::New();
    }

    assert(connection == 0);
    assert(filterIt->second);

    return filterIt->second;
}

bool GlyphMagnitudeColorMapping::usesFilter() const
{
    return true;
}

void GlyphMagnitudeColorMapping::configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper, int connection)
{
    GlyphColorMapping::configureMapper(visualizedData, mapper, connection);

    assert(m_vectorNorms.find(&visualizedData) != m_vectorNorms.end());
    assert(m_assignedVectors.find(&visualizedData) != m_assignedVectors.end());

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

    for (auto pair : m_vectorNorms)
    {
        auto && norm = pair.second;
        assert(norm && norm->GetNumberOfInputConnections(0) > 0);
        norm->Update();

        // At this pipeline stage, initial cell attributes are associated with the intermediate
        // point geometry.
        auto normData = norm->GetOutput()->GetPointData()->GetScalars();
        assert(normData);

        decltype(totalRange) range;
        normData->GetRange(range.data());
        totalRange.add(range);
    }

    return{ totalRange };
}
