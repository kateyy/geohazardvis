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


RenderedImageData::RenderedImageData(ImageDataObject * dataObject)
    : RenderedData(dataObject)
{
}

const ImageDataObject * RenderedImageData::imageDataObject() const
{
    assert(dynamic_cast<const ImageDataObject*>(dataObject()));
    return static_cast<const ImageDataObject*>(dataObject());
}

reflectionzeug::PropertyGroup * RenderedImageData::configGroup()
{
    return nullptr;
}

vtkProperty * RenderedImageData::createDefaultRenderProperty() const
{
    return vtkProperty::New();
}

vtkActor * RenderedImageData::createActor() const
{
    auto & input = *imageDataObject()->gridDataInput();

    double minValue = input.minMaxValue()[0];
    double maxValue = input.minMaxValue()[1];


    VTK_CREATE(vtkLookupTable, lut);
    lut->SetTableRange(minValue, maxValue);
    lut->SetNumberOfColors(static_cast<vtkIdType>(std::ceil(maxValue - minValue)) * 10);
    lut->SetHueRange(0.66667, 0.0);
    lut->SetValueRange(0.9, 0.9);
    lut->SetSaturationRange(1.0, 1.0);
    lut->SetAlphaRange(1.0, 1.0);
    lut->Build();

    VTK_CREATE(vtkTexture, texture);
    texture->SetLookupTable(lut);
    texture->SetInputData(input.data());
    texture->MapColorScalarsThroughLookupTableOn();
    texture->InterpolateOn();
    texture->SetQualityTo32Bit();

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
    actor->SetTexture(texture);

    return actor;
}

void RenderedImageData::updateScalarToColorMapping()
{

}

vtkPolyDataMapper * RenderedImageData::createMapper() const
{
    return nullptr;
}
