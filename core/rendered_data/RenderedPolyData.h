#pragma once

#include <QString>

#include <core/rendered_data/RenderedData3D.h>


class vtkActor;
class vtkAlgorithm;
class vtkAlgorithmOutput;
class vtkPolyDataMapper;
class vtkPolyDataNormals;

class PolyDataObject;


class CORE_API RenderedPolyData : public RenderedData3D
{
public:
    explicit RenderedPolyData(PolyDataObject & dataObject);

    PolyDataObject & polyDataObject();
    const PolyDataObject & polyDataObject() const;

    reflectionzeug::PropertyGroup * createConfigGroup() override;

    const QString & texture() const;
    void setTexture(const QString & fileName);

    vtkActor * mainActor();

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkSmartPointer<vtkProp3DCollection> fetchViewProps3D() override;

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

    QString m_textureFileName;
};
