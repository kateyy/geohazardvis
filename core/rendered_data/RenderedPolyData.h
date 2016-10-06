#pragma once

#include <QString>

#include <core/rendered_data/RenderedData3D.h>


class vtkActor;
class vtkAlgorithm;
class vtkCellCenters;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkPolyDataNormals;

class PolyDataObject;


class CORE_API RenderedPolyData : public RenderedData3D
{
public:
    explicit RenderedPolyData(PolyDataObject & dataObject);
    ~RenderedPolyData() override;

    PolyDataObject & polyDataObject();
    const PolyDataObject & polyDataObject() const;

    /** Return cell centers transformed to the coordinate system specified by setDefaultCoordinateSystem() */
    vtkAlgorithmOutput * transformedCellCenterOutputPort();
    vtkDataSet * transformedCellCenterDataSet();

    std::unique_ptr<reflectionzeug::PropertyGroup> createConfigGroup() override;

    const QString & texture() const;
    void setTexture(const QString & fileName);

    vtkActor * mainActor();

    vtkSmartPointer<vtkProperty> createDefaultRenderProperty() const override;

protected:
    vtkSmartPointer<vtkProp3DCollection> fetchViewProps3D() override;

    void scalarsForColorMappingChangedEvent() override;
    void colorMappingGradientChangedEvent() override;
    void visibilityChangedEvent(bool visible) override;

    DataBounds updateVisibleBounds() override;

    /** Integrate last pipeline steps. By default, this connects the colorMappingOutput
        with the mapper. */
    virtual void finalizePipeline();

private:
    vtkSmartPointer<vtkPolyDataMapper> m_mapper;
    vtkSmartPointer<vtkActor> m_mainActor;

    vtkSmartPointer<vtkPolyDataNormals> m_normals;      // required for lighting/interpolation

    vtkSmartPointer<vtkAlgorithm> m_colorMappingOutput;

    vtkSmartPointer<vtkCellCenters> m_transformedCellCenters;

    QString m_textureFileName;

private:
    Q_DISABLE_COPY(RenderedPolyData)
};
