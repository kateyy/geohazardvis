#pragma once

#include <core/rendered_data/RenderedData.h>


class vtkImageProperty;
class vtkImageSlice;
class vtkImageSliceMapper;

class ImageDataObject;


class CORE_API RenderedImageData : public RenderedData
{
public:
    RenderedImageData(ImageDataObject * dataObject);

    reflectionzeug::PropertyGroup * createConfigGroup() override;

protected:
    vtkSmartPointer<vtkPropCollection> fetchViewProps() override;
    vtkImageSlice * slice();

    vtkImageProperty * property();

    void colorMappingGradientChangedEvent() override;
    void visibilityChangedEvent(bool visible) override;

private:
    vtkSmartPointer<vtkImageSliceMapper> m_mapper;
    vtkSmartPointer<vtkImageSlice> m_slice;
    vtkSmartPointer<vtkImageProperty> m_property;
};
