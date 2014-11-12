#pragma once

#include <core/rendered_data/RenderedData3D.h>


class vtkPolyDataMapper;

class PolyDataObject;


class CORE_API RenderedPolyData : public RenderedData3D
{
public:
    RenderedPolyData(PolyDataObject * dataObject);
    ~RenderedPolyData() override;

    PolyDataObject * polyDataObject();
    const PolyDataObject * polyDataObject() const;

    reflectionzeug::PropertyGroup * createConfigGroup() override;

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkSmartPointer<vtkActorCollection> fetchActors() override;

    void scalarsForColorMappingChangedEvent() override;
    void colorMappingGradientChangedEvent() override;
    void visibilityChangedEvent(bool visible) override;

private:
    vtkSmartPointer<vtkPolyDataMapper> m_mapper;
    vtkSmartPointer<vtkActor> m_mainActor;
};
