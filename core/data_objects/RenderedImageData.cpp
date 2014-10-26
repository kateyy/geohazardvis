#include "RenderedImageData.h"

#include <cassert>

#include <vtkPlaneSource.h>

#include <vtkPolyDataMapper.h>

#include <vtkProperty.h>
#include <vtkTexture.h>
#include <vtkActor.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/data_objects/ImageDataObject.h>
#include <core/vtkhelper.h>
#include <core/scalar_mapping/ScalarsForColorMapping.h>


using namespace reflectionzeug;

namespace
{
    enum class Interpolation
    {
        flat = VTK_FLAT, gouraud = VTK_GOURAUD, phong = VTK_PHONG
    };
}

RenderedImageData::RenderedImageData(ImageDataObject * dataObject)
    : RenderedData(dataObject)
    , m_texture(vtkSmartPointer<vtkTexture>::New())
{
    m_texture->SetInputConnection(dataObject->processedOutputPort());
    m_texture->MapColorScalarsThroughLookupTableOn();
    m_texture->InterpolateOn();
    m_texture->SetQualityTo32Bit();
}

const ImageDataObject * RenderedImageData::imageDataObject() const
{
    assert(dynamic_cast<const ImageDataObject*>(dataObject()));
    return static_cast<const ImageDataObject*>(dataObject());
}

reflectionzeug::PropertyGroup * RenderedImageData::createConfigGroup()
{
    PropertyGroup * renderSettings = new PropertyGroup();

    auto * interpolate = renderSettings->addProperty<bool>("interpolate",
        [this]() {
        return m_texture->GetInterpolate() != 0;
    },
        [this](bool doInterpolate) {
        m_texture->SetInterpolate(doInterpolate);
        emit geometryChanged();
    });

    /*auto * quality = renderSettings->addProperty<bool>("hq",
        [this]() {
        return m_texture->GetQuality() == VTK_TEXTURE_QUALITY_32BIT;
    },
        [this](bool hq) {
        m_texture->SetQuality(hq ? VTK_TEXTURE_QUALITY_32BIT : VTK_TEXTURE_QUALITY_16BIT);
        emit geometryChanged();
    });*/

    return renderSettings;
}

vtkProperty * RenderedImageData::createDefaultRenderProperty() const
{
    vtkProperty * property = vtkProperty::New();
    property->LightingOff();
    return property;
}

vtkActor * RenderedImageData::createActor()
{
    ImageDataObject * image = static_cast<ImageDataObject *>(dataObject());
    const int * extent = image->extent();
    int xMin = extent[0], xMax = extent[1], yMin = extent[2], yMax = extent[3];

    VTK_CREATE(vtkPlaneSource, plane);
    plane->SetXResolution(image->dimensions()[0] -1);
    plane->SetYResolution(image->dimensions()[1] -1);
    plane->SetOrigin(xMin, yMin, 0);
    plane->SetPoint1(xMax, yMin, 0);
    plane->SetPoint2(xMin, yMax, 0);

    VTK_CREATE(vtkPolyDataMapper, planeMapper);
    planeMapper->SetInputConnection(plane->GetOutputPort());

    vtkActor * actor = vtkActor::New();
    actor->SetMapper(planeMapper);
    actor->SetTexture(m_texture);

    return actor;
}

void RenderedImageData::scalarsForColorMappingChangedEvent()
{
    bool enableColorMapping = m_scalars && (m_scalars->dataMinValue() != m_scalars->dataMaxValue());

    if (enableColorMapping)
        mainActor()->SetTexture(m_texture);
    else
        mainActor()->SetTexture(nullptr);
}

void RenderedImageData::colorMappingGradientChangedEvent()
{
    m_texture->SetLookupTable(m_gradient);
}
