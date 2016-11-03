#include "GlyphMagnitudeColorMapping.h"

#include <cassert>

#include <vtkAssignAttribute.h>
#include <vtkCellData.h>
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


namespace
{
    const QString s_name = "Glyph Magnitude";

    const char * const s_normScalarsName = "VectorNorm";
}

const bool GlyphMagnitudeColorMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name, newInstances);


std::vector<std::unique_ptr<ColorMappingData>> GlyphMagnitudeColorMapping::newInstances(const QList<AbstractVisualizedData*> & visualizedData)
{
    QMap<QString, QList<GlyphMappingData *>> glyphMappings;
    for (auto vis : visualizedData)
    {
        auto data = dynamic_cast<RenderedData3D *>(vis);
        if (!data)
        {
            continue;
        }

        for (auto & vectorData : data->glyphMapping().vectors())
        {
            if (vectorData->isVisible())
            {
                glyphMappings[vectorData->name()] << vectorData;
            }
        }
    }

    std::vector<std::unique_ptr<ColorMappingData>> instances;
    for (auto it = glyphMappings.begin(); it != glyphMappings.end(); ++it)
    {
        auto instance = std::make_unique<GlyphMagnitudeColorMapping>(visualizedData, it.value(), it.key());
        if (instance->isValid())
        {
            instance->initialize();
            instances.push_back(std::move(instance));
        }
    }

    return instances;
}

GlyphMagnitudeColorMapping::GlyphMagnitudeColorMapping(
    const QList<AbstractVisualizedData *> & visualizedData,
    const QList<GlyphMappingData *> & glyphMappingData,
    const QString & vectorsName)
    : GlyphColorMapping(visualizedData, glyphMappingData, 1)
    , m_vectorName{ vectorsName }
{
    for (auto glyphMapping : glyphMappingData)
    {
        auto norm = vtkSmartPointer<vtkVectorNorm>::New();
        norm->SetInputConnection(glyphMapping->vectorDataOutputPort());

        auto assignVectors = vtkSmartPointer<vtkAssignAttribute>::New();
        assignVectors->Assign(vectorsName.toUtf8().data(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);
        assignVectors->SetInputConnection(norm->GetOutputPort());

        auto setArrayName = vtkSmartPointer<ArrayChangeInformationFilter>::New();
        setArrayName->EnableRenameOn();
        setArrayName->SetArrayName(s_normScalarsName);
        setArrayName->SetAttributeType(vtkDataSetAttributes::SCALARS);
        setArrayName->SetAttributeLocation(ArrayChangeInformationFilter::POINT_DATA);
        setArrayName->SetInputConnection(assignVectors->GetOutputPort());

        // if needed: support multiple connections
        m_vectorNorms.insert(&glyphMapping->renderedData(), { norm });
        m_assignedVectors.insert(&glyphMapping->renderedData(), { setArrayName });

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

IndexType GlyphMagnitudeColorMapping::scalarsAssociation(AbstractVisualizedData & /*vis*/) const
{
    return IndexType::points;
}

vtkSmartPointer<vtkAlgorithm> GlyphMagnitudeColorMapping::createFilter(AbstractVisualizedData & visualizedData, int connection)
{
    auto & filters = m_assignedVectors.value(&visualizedData, {});

    if (filters.empty())  // required/valid filters are already created
    {
        return vtkSmartPointer<vtkPassThrough>::New();
    }

    assert(filters.size() > connection);
    auto filter = filters[connection];
    assert(filter);

    return filter;
}

bool GlyphMagnitudeColorMapping::usesFilter() const
{
    return true;
}

void GlyphMagnitudeColorMapping::configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper, int connection)
{
    GlyphColorMapping::configureMapper(visualizedData, mapper, connection);

    assert(m_vectorNorms.contains(&visualizedData));
    assert(m_assignedVectors.contains(&visualizedData));

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

    for (auto norms : m_vectorNorms.values())
    {
        for (auto norm : norms)
        {
            assert(norm && norm->GetNumberOfInputConnections(0) > 0);
            norm->Update();
            vtkDataArray * normData =
                norm->GetOutput()->GetPointData()->GetScalars();
            if (!normData)
                normData = norm->GetOutput()->GetCellData()->GetScalars();
            assert(normData);

            decltype(totalRange) range;
            normData->GetRange(range.data());
            totalRange.add(range);
        }
    }

    return{ totalRange };
}
