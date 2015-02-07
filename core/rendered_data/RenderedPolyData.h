#pragma once

#include <core/rendered_data/RenderedData3D.h>


class vtkAlgorithm;
class vtkAlgorithmOutput;
class vtkPolyDataMapper;
class vtkPolyDataNormals;

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
    vtkSmartPointer<vtkProp3DCollection> fetchViewProps3D() override;
    vtkActor * mainActor();

    void scalarsForColorMappingChangedEvent() override;
    void colorMappingGradientChangedEvent() override;
    void visibilityChangedEvent(bool visible) override;

    /** Integrate last pipeline steps. By default, this connects the colorMappingOutput
        with the mapper. */
    virtual void finalizePipeline();

    vtkAlgorithmOutput * colorMappingOutput();
    vtkPolyDataMapper * mapper();

private:
    vtkSmartPointer<vtkPolyDataMapper> m_mapper;
    vtkSmartPointer<vtkActor> m_mainActor;

    vtkSmartPointer<vtkPolyDataNormals> m_normals;      // required for lighting/interpolation

    vtkSmartPointer<vtkAlgorithmOutput> m_colorMappingOutput;
};
