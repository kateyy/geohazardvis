#include "RenderedImageData.h"

#include <cassert>

#include <vtkLookupTable.h>

#include <vtkDataSet.h>

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
    m_texture->SetInputData(dataObject->dataSet());
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
    PropertyGroup * configGroup = new PropertyGroup();

    auto * renderSettings = new PropertyGroup("renderSettings");
    renderSettings->setOption("title", "rendering");
    renderSettings->setOption("title", "rendering");
    configGroup->addProperty(renderSettings);

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

    return configGroup;
}

vtkProperty * RenderedImageData::createDefaultRenderProperty() const
{
    return vtkProperty::New();
}

vtkActor * RenderedImageData::createActor()
{
    const double * bounds = dataObject()->bounds();
    double xExtend = bounds[1] - bounds[0];
    double yExtend = bounds[3] - bounds[2];

    ImageDataObject * image = static_cast<ImageDataObject*>(dataObject());

    VTK_CREATE(vtkPlaneSource, plane);
    plane->SetXResolution(image->dimensions()[0]);
    plane->SetYResolution(image->dimensions()[1]);
    plane->SetOrigin(bounds[0], bounds[2], 0);
    plane->SetPoint1(bounds[0] + xExtend, bounds[2], 0);
    plane->SetPoint2(bounds[0], bounds[2] + yExtend, 0);

    VTK_CREATE(vtkPolyDataMapper, planeMapper);
    planeMapper->SetInputConnection(plane->GetOutputPort());

    vtkActor * actor = vtkActor::New();
    actor->SetMapper(planeMapper);
    actor->SetTexture(m_texture);

    return actor;
}

void RenderedImageData::scalarsForColorMappingChangedEvent()
{
    updateTexture();
}

void RenderedImageData::gradientForColorMappingChangedEvent()
{
    updateTexture();
}

vtkPolyDataMapper * RenderedImageData::createMapper() const
{
    return nullptr;
}

void RenderedImageData::updateTexture()
{
    if (!m_lut)
        return;

    // not update needed for gradient == nullptr
    // this happens only when the ScalarMappingChooser / mapping is not fully initialized

    m_lut->SetTableRange(m_scalars->minValue(), m_scalars->maxValue());
    m_texture->SetLookupTable(m_lut);
}
