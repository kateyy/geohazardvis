#pragma once

#include <core/data_objects/RenderedData.h>


class QImage;

class vtkPolyDataMapper;

class PolyDataObject;


class CORE_API RenderedPolyData : public RenderedData
{
public:
    RenderedPolyData(PolyDataObject * dataObject);
    ~RenderedPolyData() override;

    PolyDataObject * polyDataObject();
    const PolyDataObject * polyDataObject() const;

    reflectionzeug::PropertyGroup * createConfigGroup() override;

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkActor * createActor() override;

    void scalarsForColorMappingChangedEvent() override;
    void colorMappingGradientChangedEvent() override;

private:
    vtkPolyDataMapper * createDataMapper();
};
