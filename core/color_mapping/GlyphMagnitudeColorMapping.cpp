#include "GlyphMagnitudeColorMapping.h"

#include <cassert>

#include <QSet>

#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkLookupTable.h>
#include <vtkMapper.h>
#include <vtkPassThrough.h>
#include <vtkPointData.h>
#include <vtkVectorNorm.h>

#include <core/vtkhelper.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/rendered_data/RenderedData3D.h>
#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>


namespace
{
    const QString s_name = "Glyph Magnitude";
}

const bool GlyphMagnitudeColorMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name, newInstances);


QList<ColorMappingData *> GlyphMagnitudeColorMapping::newInstances(const QList<AbstractVisualizedData*> & visualizedData)
{
    QMap<QString, QList<GlyphMappingData *>> glyphMappings;
    for (auto vis : visualizedData)
    {
        RenderedData3D * data = dynamic_cast<RenderedData3D *>(vis);
        if (!data)
            continue;

        const QMap<QString, GlyphMappingData *> & vectors = data->glyphMapping()->vectors();
        for (auto it = vectors.begin(); it != vectors.end(); ++it)
        {
            GlyphMappingData * vectorData = it.value();
            if (vectorData->isVisible())
                glyphMappings[vectorData->name()] << vectorData;
        }
    }

    QList<ColorMappingData *> instances;
    for (auto it = glyphMappings.begin(); it != glyphMappings.end(); ++it)
    {
        auto instance = new GlyphMagnitudeColorMapping(visualizedData, it.value(), it.key());
        if (instance->isValid())
        {
            instance->initialize();
            instances << instance;
        }
        else
            delete instance;
    }

    return instances;
}

GlyphMagnitudeColorMapping::GlyphMagnitudeColorMapping(
    const QList<AbstractVisualizedData *> & visualizedData,
    const QList<GlyphMappingData *> & glyphMappingData,
    const QString & vectorsName)
    : GlyphColorMapping(visualizedData, glyphMappingData, 1)
    , m_vectorName(vectorsName)
{
    for (GlyphMappingData * glyphMapping : glyphMappingData)
    {
        VTK_CREATE(vtkVectorNorm, norm);
        norm->SetInputConnection(glyphMapping->vectorDataOutputPort());
        // if needed: support multiple connections
        m_vectorNorms.insert(glyphMapping->renderedData(), { norm });

        m_isValid = true;
    }
}

GlyphMagnitudeColorMapping::~GlyphMagnitudeColorMapping()
{
}

QString GlyphMagnitudeColorMapping::name() const
{
    return "Glyph Magnitude: " + m_vectorName;
}

vtkSmartPointer<vtkAlgorithm> GlyphMagnitudeColorMapping::createFilter(AbstractVisualizedData * visualizedData, int connection)
{
    auto & filters = m_vectorNorms.value(visualizedData, {});
    
    if (filters.isEmpty())  // required/valid filters are already created
        return vtkPassThrough::New();

    assert(filters.size() > connection);
    auto filter = filters[connection];
    assert(filter);

    return filter;
}

bool GlyphMagnitudeColorMapping::usesFilter() const
{
    return true;
}

void GlyphMagnitudeColorMapping::configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * mapper)
{
    GlyphColorMapping::configureMapper(visualizedData, mapper);

    assert(m_vectorNorms.contains(visualizedData));

    if (auto m = vtkMapper::SafeDownCast(mapper))
    {
        m->UseLookupTableScalarRangeOn();
        m->SetLookupTable(m_lut);
        m->ScalarVisibilityOn();
    }
}

QMap<vtkIdType, QPair<double, double>> GlyphMagnitudeColorMapping::updateBounds()
{
    double totalMin = std::numeric_limits<double>::max();
    double totalMax = std::numeric_limits<double>::lowest();

    for (auto norms : m_vectorNorms.values())
    {
        for (vtkVectorNorm * norm : norms)
        {
            assert(norm && norm->GetNumberOfInputConnections(0) > 0);
            norm->Update();
            vtkDataArray * normData =
                norm->GetOutput()->GetPointData()->GetScalars();
            if (!normData)
                norm->GetOutput()->GetCellData()->GetScalars();
            assert(normData);

            double range[2];
            normData->GetRange(range);
            totalMin = std::min(totalMin, range[0]);
            totalMax = std::max(totalMax, range[1]);
        }
    }

    return{ { 0, { totalMin, totalMax } } };
}
