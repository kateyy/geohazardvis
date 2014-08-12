#include "RenderedImageData.h"

#include <cassert>

#include <vtkLookupTable.h>

#include <vtkDataSet.h>

#include <vtkPlaneSource.h>

#include <vtkPolyDataMapper.h>

#include <vtkProperty.h>
#include <vtkTexture.h>
#include <vtkActor.h>

#include <core/data_objects/ImageDataObject.h>
#include <core/Input.h>
#include <core/vtkhelper.h>
#include <core/data_mapping/ScalarsForColorMapping.h>


RenderedImageData::RenderedImageData(ImageDataObject * dataObject)
    : RenderedData(dataObject)
    , m_texture(vtkSmartPointer<vtkTexture>::New())
{
    m_texture->SetInputData(imageDataObject()->gridDataInput()->data());
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
    return nullptr;
}

vtkProperty * RenderedImageData::createDefaultRenderProperty() const
{
    return vtkProperty::New();
}

vtkActor * RenderedImageData::createActor()
{
    auto & input = *imageDataObject()->gridDataInput();

    double xExtend = input.bounds()[1] - input.bounds()[0];
    double yExtend = input.bounds()[3] - input.bounds()[2];

    VTK_CREATE(vtkPlaneSource, plane);
    plane->SetXResolution(input.dimensions()[0]);
    plane->SetYResolution(input.dimensions()[1]);
    plane->SetOrigin(input.bounds()[0], input.bounds()[2], 0);
    plane->SetPoint1(input.bounds()[0] + xExtend, input.bounds()[2], 0);
    plane->SetPoint2(input.bounds()[0], input.bounds()[2] + yExtend, 0);

    vtkPolyDataMapper * planeMapper = vtkPolyDataMapper::New();
    planeMapper->SetInputConnection(plane->GetOutputPort());

    vtkActor * actor = vtkActor::New();
    actor->SetMapper(planeMapper);
    actor->SetTexture(m_texture);

    return actor;
}

void RenderedImageData::scalarsForColorMappingChangedEvent()
{
    updateTexture();
    
    if (m_scalars)
        connect(m_scalars, &ScalarsForColorMapping::minMaxChanged,
            [this]()
            {
                if (m_lut)
                    m_lut->SetTableRange(m_scalars->minValue(), m_scalars->maxValue());
            });
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
    // this happens only when the DataChooser / mapping is not fully initialized

    m_lut->SetTableRange(m_scalars->minValue(), m_scalars->maxValue());
    m_texture->SetLookupTable(m_lut);
}
