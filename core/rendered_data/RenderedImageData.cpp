#include "RenderedImageData.h"

#include <cassert>

#include <vtkAssignAttribute.h>
#include <vtkDataSet.h>
#include <vtkImageMapToColors.h>
#include <vtkImageSliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkImageProperty.h>
#include <vtkLookupTable.h>
#include <vtkPointData.h>
#include <vtkPropCollection.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/types.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/color_mapping/ColorMappingData.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/filters/ArrayChangeInformationFilter.h>
#include <core/filters/DEMApplyShadingToColors.h>
#include <core/filters/DEMImageNormals.h>
#include <core/filters/DEMShadingFilter.h>


using namespace reflectionzeug;


RenderedImageData::RenderedImageData(ImageDataObject & dataObject)
    : RenderedData(ContentType::Rendered2D, dataObject)
    , m_isShadingEnabled{ false }
    , m_mapper{ vtkSmartPointer<vtkImageSliceMapper>::New() } // replace with vtkImageResliceMapper?
    , m_demShading{ vtkSmartPointer<DEMShadingFilter>::New() }
    , m_property{ vtkSmartPointer<vtkImageProperty>::New() }
{
    m_mapper->SetInputConnection(dataObject.processedOutputPort());
    m_property->UseLookupTableScalarRangeOn();
    // Linear interpolation is not working correctly, at least not on the legacy OpenGL backend
    m_property->SetInterpolationTypeToCubic();

    setupInformation(*m_mapper->GetInformation(), *this);
}

ImageDataObject & RenderedImageData::imageDataObject()
{
    assert(dynamic_cast<ImageDataObject *>(&dataObject()));
    return static_cast<ImageDataObject &>(dataObject());
}

const ImageDataObject & RenderedImageData::imageDataObject() const
{
    assert(dynamic_cast<const ImageDataObject *>(&dataObject()));
    return static_cast<const ImageDataObject &>(dataObject());
}

std::unique_ptr<PropertyGroup> RenderedImageData::createConfigGroup()
{
    auto renderSettings = RenderedData::createConfigGroup();

    renderSettings->addProperty<bool>("Interpolation",
        [this] () {
        return isInterpolationEnabled();
    },
        [this] (bool enable) {
        setInterpolationEnabled(enable);
        emit geometryChanged();
    });

    renderSettings->addProperty<double>("Transparency",
        [this] () {
        return (1.0 - property()->GetOpacity()) * 100;
    },
        [this] (double transparency) {
        property()->SetOpacity(1.0 - transparency * 0.01);
        emit geometryChanged();
    })
        ->setOptions({
            { "minimum", 0 },
            { "maximum", 100 },
            { "step", 1 },
            { "suffix", " %" }
    });

    const bool mightBeADEM = imageDataObject().scalars().GetNumberOfComponents() == 1;

    if (!mightBeADEM)
    {
        return renderSettings;
    }

    renderSettings->addProperty<bool>("DEMShading",
        [this] () { return isShadingEnabled(); },
        [this] (bool enable) {
        if (enable == isShadingEnabled())
        {
            return;
        }
        setEnableShading(enable);
        emit geometryChanged();
    })
        ->setOption("title", "DEM Shading");

    renderSettings->addProperty<double>("Ambient",
        [this] () { return ambient(); },
        [this] (double a) {
        if (a == ambient())
        {
            return;
        }
        setAmbient(a);
        emit geometryChanged();
    })
        ->setOptions({
            { "title", "Ambient Light" },
            { "minimum", 0.0 },
            { "maximum", 1.0 },
            { "step", 0.01 }
    });

    renderSettings->addProperty<double>("Diffuse",
        [this] () { return diffuse(); },
        [this] (double d) {
        if (d == diffuse())
        {
            return;
        }
        setDiffuse(d);
        emit geometryChanged();
    })
        ->setOptions({
            { "title", "Diffuse Light" },
            { "minimum", 0.0 },
            { "maximum", 1.0 },
            { "step", 0.01 }
    });

    return renderSettings;
}

bool RenderedImageData::isInterpolationEnabled() const
{
    return m_property->GetInterpolationType() != VTK_NEAREST_INTERPOLATION;
}

void RenderedImageData::setInterpolationEnabled(bool enable)
{
    if (isInterpolationEnabled() == enable)
    {
        return;
    }

    m_property->SetInterpolationType(enable
        ? VTK_CUBIC_INTERPOLATION
        : VTK_NEAREST_INTERPOLATION);
}

bool RenderedImageData::isShadingEnabled() const
{
    return m_isShadingEnabled;
}

void RenderedImageData::setEnableShading(bool enable)
{
    if (m_isShadingEnabled == enable)
    {
        return;
    }

    m_isShadingEnabled = enable;

    configureVisPipeline();
}

double RenderedImageData::ambient() const
{
    return m_demShading->GetAmbient();
}

void RenderedImageData::setAmbient(double ambient)
{
    m_demShading->SetAmbient(ambient);    
}

double RenderedImageData::diffuse() const
{
    return m_demShading->GetDiffuse();
}

void RenderedImageData::setDiffuse(double diffuse)
{
    m_demShading->SetDiffuse(diffuse);
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

    m_colorMappingFilter = {};

    if (m_colorMappingData && m_colorMappingData->usesFilter())
    {
        auto filter = m_colorMappingData->createFilter(*this);
        filter->Update();

        auto image = vtkDataSet::SafeDownCast(filter->GetOutputDataObject(0));
        // use filter only if it outputs scalars. To prevent segfault in image slice.
        if (image->GetPointData()->GetScalars())
        {
            m_colorMappingFilter = filter;
        }
    }

    configureVisPipeline();
}

void RenderedImageData::colorMappingGradientChangedEvent()
{
    RenderedData::colorMappingGradientChangedEvent();

    configureVisPipeline();
}

void RenderedImageData::visibilityChangedEvent(bool visible)
{
    RenderedData::visibilityChangedEvent(visible);

    slice()->SetVisibility(visible);
}

void RenderedImageData::initializePipeline()
{
    if (m_copyScalarsFilter)
    {
        return;
    }

    // Assume immutable scalars name
    const auto scalarsName = imageDataObject().scalars().GetName();
    const auto mappedColorsName = (QString::fromUtf8(scalarsName) + " (RGBA)").toUtf8();

    // vtkImageMapToColors consumes the scalars, but we still need them in downstream filters
    m_copyScalarsFilter = vtkSmartPointer<ArrayChangeInformationFilter>::New();
    m_copyScalarsFilter->PassInputArrayOn();
    m_copyScalarsFilter->EnableRenameOn();
    m_copyScalarsFilter->SetArrayName(mappedColorsName.data());

    m_imageScalarsToColors = vtkSmartPointer<vtkImageMapToColors>::New();
    m_imageScalarsToColors->SetInputConnection(m_copyScalarsFilter->GetOutputPort());

    m_assignElevationsForNormalComputation = vtkSmartPointer<vtkAssignAttribute>::New();
    m_assignElevationsForNormalComputation->Assign(scalarsName,
        vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);

    m_demNormals = vtkSmartPointer<DEMImageNormals>::New();
    m_demNormals->SetInputConnection(m_assignElevationsForNormalComputation->GetOutputPort());

    assert(m_demShading);
    m_demShading->SetInputConnection(m_demNormals->GetOutputPort());

    m_assignMappedColorsForShading = vtkSmartPointer<vtkAssignAttribute>::New();
    m_assignMappedColorsForShading->SetInputConnection(m_demShading->GetOutputPort());
    m_assignMappedColorsForShading->Assign(mappedColorsName.data(),
        vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);

    m_applyDEMShading = vtkSmartPointer<DEMApplyShadingToColors>::New();
}

void RenderedImageData::configureVisPipeline()
{
    initializePipeline();

    /**
    Determine the current source data for visualization.
    In the simplest cast, this is the output of dataObject() (which is colorMappingInput()).
    Depending on the configuration, the visualization pipeline is extended by a
    - color mapping filter
    - vtkImageMapToColors, to be able to mix mapped colors with shading
    - shading filter
    */

    vtkSmartPointer<vtkAlgorithm> colorMappingFilter;
    vtkAlgorithmOutput * currentPipelineStep = colorMappingInput();
    bool mapScalarsToColors = false;
    vtkSmartPointer<vtkScalarsToColors> lut = m_gradient;
    int component = 0;

    if (m_colorMappingFilter)
    {
        currentPipelineStep = m_colorMappingFilter->GetOutputPort();

        assert(m_colorMappingData);
        m_colorMappingData->configureMapper(*this, *m_mapper);
        component = m_colorMappingData->dataComponent();

        mapScalarsToColors = m_colorMappingData->mapsScalarsToColors();

        if (m_colorMappingData->usesOwnLookupTable())
        {
            lut = m_colorMappingData->ownLookupTable();
        }
    }

    if (mapScalarsToColors)
    {
        m_copyScalarsFilter->SetInputConnection(currentPipelineStep);

        m_imageScalarsToColors->SetLookupTable(lut);
        m_imageScalarsToColors->SetActiveComponent(component);
        currentPipelineStep = m_imageScalarsToColors->GetOutputPort();
    }

    if (isShadingEnabled())
    {
        m_assignElevationsForNormalComputation->SetInputConnection(currentPipelineStep);

        if (mapScalarsToColors)
        {
            m_applyDEMShading->SetInputConnection(m_assignMappedColorsForShading->GetOutputPort());
        }
        else
        {
            m_applyDEMShading->SetInputConnection(m_demShading->GetOutputPort());
        }

        currentPipelineStep = m_applyDEMShading->GetOutputPort();
    }

    m_mapper->SetInputConnection(currentPipelineStep);

    // hide the image slice if not using color mapping or shading
    property()->SetOpacity(
        colorMapping().isEnabled() || isShadingEnabled()
        ? 1.0
        : 0.0);
}
