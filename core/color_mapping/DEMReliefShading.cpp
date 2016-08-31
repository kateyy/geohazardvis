#include "DEMReliefShading.h"

#include <cassert>

#include <vtkAbstractMapper.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkLookupTable.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>

#include <core/AbstractVisualizedData.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/filters/DEMImageNormals.h>
#include <core/filters/DEMShadingFilter.h>
#include <core/utility/type_traits.h>
#include <core/utility/DataExtent.h>


namespace
{
const QString l_name = "DEM relief shading";
const QString l_dataArrayName = "DEM Relief Shading";


}

const bool DEMReliefShading::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    l_name,
    &DEMReliefShading::newInstance);

std::vector<std::unique_ptr<ColorMappingData>> DEMReliefShading::newInstance(const QList<AbstractVisualizedData *> & visualizedData)
{
    Unqualified<decltype(visualizedData)> validVisualizations;

    for (auto vis : visualizedData)
    {
        if (auto img = dynamic_cast<ImageDataObject *>(&vis->dataObject()))
        {
            auto scalars = img->dataSet()->GetPointData()->GetScalars();
            if (!scalars || !scalars->GetName() || (scalars->GetName() == '\0'))
            {
                continue;
            }

            validVisualizations << vis;
        }
    }

    decltype(newInstance(visualizedData)) result;
    if (!validVisualizations.empty())
    {
        result.push_back(std::make_unique<DEMReliefShading>(validVisualizations));
    }

    return result;
}

DEMReliefShading::DEMReliefShading(const QList<AbstractVisualizedData*> & visualizedData)
    : ColorMappingData(visualizedData, 1, true, true)
{
    assert(!visualizedData.isEmpty());

    m_isValid = true;
}

DEMReliefShading::~DEMReliefShading() = default;

QString DEMReliefShading::name() const
{
    return l_dataArrayName;
}

QString DEMReliefShading::scalarsName() const
{
    return l_dataArrayName;
}

vtkSmartPointer<vtkAlgorithm> DEMReliefShading::createFilter(AbstractVisualizedData * visualizedData, int connection)
{
    static const auto arrayName = scalarsName().toUtf8();

    if (m_filters.contains(visualizedData))
    {
        if (m_filters[visualizedData].contains(connection))
        {
            return m_filters[visualizedData][connection];
        }
    }

    auto normals = vtkSmartPointer<DEMImageNormals>::New();
    normals->SetInputConnection(visualizedData->colorMappingInput(connection));

    auto shading = vtkSmartPointer<DEMShadingFilter>::New();
    shading->SetInputConnection(normals->GetOutputPort());

    m_filters[visualizedData][connection] = shading;

    return shading;
}

bool DEMReliefShading::usesFilter() const
{
    return true;
}

std::vector<ValueRange<>> DEMReliefShading::updateBounds()
{
    return{ ValueRange<>({ 0, 1 }) };
}

vtkSmartPointer<vtkScalarsToColors> DEMReliefShading::createOwnLookupTable()
{
    auto lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetTableRange(0.0, 1.0);
    // gray scale mapping
    lut->SetValueRange(0.0, 1.0);
    lut->SetHueRange(0.0, 0.0);
    lut->SetSaturationRange(0.0, 0.0);
    lut->SetAlphaRange(1.0, 1.0);
    lut->Build();

    return lut;
}

void DEMReliefShading::lookupTableChangedEvent()
{
    ColorMappingData::lookupTableChangedEvent();

    auto ownLut = vtkLookupTable::SafeDownCast(ownLookupTable());
    assert(ownLut);

    ownLut->SetNanColor(m_lut->GetNanColor());
}

void DEMReliefShading::minMaxChangedEvent()
{
    ColorMappingData::minMaxChangedEvent();

    for (auto ports : m_filters)
    {
        for (auto filter : ports)
        {
            DEMShadingFilter::SafeDownCast(filter)->SetAmbientRatio(minValue());
        }
    }
}
