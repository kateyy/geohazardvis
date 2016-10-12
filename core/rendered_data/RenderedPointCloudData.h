#pragma once

#include <QString>

#include <core/rendered_data/RenderedData3D.h>


class vtkActor;
class vtkAlgorithm;
class vtkPolyData;
class vtkPolyDataMapper;

class PointCloudDataObject;


class CORE_API RenderedPointCloudData : public RenderedData3D
{
public:
    explicit RenderedPointCloudData(PointCloudDataObject & dataObject);
    ~RenderedPointCloudData() override;

    PointCloudDataObject & pointCloudDataObject();
    const PointCloudDataObject & pointCloudDataObject() const;

    std::unique_ptr<reflectionzeug::PropertyGroup> createConfigGroup() override;

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

    vtkSmartPointer<vtkAlgorithm> m_colorMappingOutput;

private:
    Q_DISABLE_COPY(RenderedPointCloudData)
};
