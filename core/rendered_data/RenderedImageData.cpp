#include "RenderedImageData.h"

#include <cassert>

#include <vtkInformation.h>

#include <vtkPropCollection.h>

#include <vtkLookupTable.h>

#include <vtkDataSet.h>
#include <vtkImageSliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkImageProperty.h>
#include <vtkPointData.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/types.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/color_mapping/ColorMappingData.h>
#include <core/data_objects/ImageDataObject.h>


using namespace reflectionzeug;


RenderedImageData::RenderedImageData(ImageDataObject & dataObject)
    : RenderedData(ContentType::Rendered2D, dataObject)
    , m_mapper{ vtkSmartPointer<vtkImageSliceMapper>::New() } // replace with vtkImageResliceMapper?
{
    m_mapper->SetInputConnection(dataObject.processedOutputPort());

    setupInformation(*m_mapper->GetInformation(), *this);
}

std::unique_ptr<PropertyGroup> RenderedImageData::createConfigGroup()
{
    auto renderSettings = RenderedData::createConfigGroup();

    auto prop_interpolation = renderSettings->addProperty<Interpolation>("Interpolation",
        [this] () {
        return static_cast<Interpolation>(property()->GetInterpolationType());
    },
        [this] (Interpolation interpolation) {
        property()->SetInterpolationType(static_cast<int>(interpolation));
        emit geometryChanged();
    });
    prop_interpolation->setStrings({
            { Interpolation::nearest, "nearest" },
            { Interpolation::linear, "linear" },
            { Interpolation::cubic, "cubic" }
    });


    auto transparency = renderSettings->addProperty<double>("Transparency",
        [this] () {
        return (1.0 - property()->GetOpacity()) * 100;
    },
        [this] (double transparency) {
        property()->SetOpacity(1.0 - transparency * 0.01);
        emit geometryChanged();
    });
    transparency->setOption("minimum", 0);
    transparency->setOption("maximum", 100);
    transparency->setOption("step", 1);
    transparency->setOption("suffix", " %");

    return renderSettings;
}

vtkSmartPointer<vtkPropCollection> RenderedImageData::fetchViewProps()
{
    auto props = vtkSmartPointer<vtkPropCollection>::New();
    props->AddItem(slice());

    return props;
}

vtkImageSlice * RenderedImageData::slice()
{
    if (!m_slice)
    {
        m_slice = vtkSmartPointer<vtkImageSlice>::New();
        m_slice->SetMapper(m_mapper);
        m_slice->SetProperty(property());
    }

    return m_slice;
}

vtkImageProperty * RenderedImageData::property()
{
    if (!m_property)
    {
        m_property = vtkSmartPointer<vtkImageProperty>::New();
        m_property->UseLookupTableScalarRangeOn();
    }

    return m_property;
}

void RenderedImageData::setupColorMapping(ColorMapping & colorMapping)
{
    RenderedData::setupColorMapping(colorMapping);

    auto & image = static_cast<ImageDataObject &>(dataObject());
    auto & scalars = image.scalars();

    // visualizing images without color mapping doesn't make sense in most cases
    colorMapping.setCurrentScalarsByName(QString::fromUtf8(scalars.GetName()), true);
}

void RenderedImageData::scalarsForColorMappingChangedEvent()
{
    RenderedData::scalarsForColorMappingChangedEvent();

    if (!m_colorMappingData)
    {
        m_mapper->SetInputConnection(colorMappingInput());
        return;
    }

    m_colorMappingData->configureMapper(this, m_mapper);

    if (m_colorMappingData->usesFilter())
    {
        auto filter = m_colorMappingData->createFilter(this);
        filter->Update();
        auto image = vtkDataSet::SafeDownCast(filter->GetOutputDataObject(0));
        // use filter only if it outputs scalars. To prevent segfault in image slice.
        if (image->GetPointData()->GetScalars())
            m_mapper->SetInputConnection(filter->GetOutputPort());
        else
            m_mapper->SetInputConnection(colorMappingInput());
    }
    else
    {
        m_mapper->SetInputConnection(colorMappingInput());
    }

    property()->SetOpacity(colorMapping().isEnabled()
        ? 1.0
        : 0.0);

    // hack required for DirectImageColors: how to enforce to use color data directly?
    auto lut = m_colorMappingData->mapsScalarsToColors()
        ? m_gradient : nullptr;
    if (m_colorMappingData->usesOwnLookupTable())
    {
        lut = m_colorMappingData->ownLookupTable();
    }
    property()->SetLookupTable(lut);
}

void RenderedImageData::colorMappingGradientChangedEvent()
{
    RenderedData::colorMappingGradientChangedEvent();

    property()->SetLookupTable(m_gradient);
}

void RenderedImageData::visibilityChangedEvent(bool visible)
{
    RenderedData::visibilityChangedEvent(visible);

    slice()->SetVisibility(visible);
}
