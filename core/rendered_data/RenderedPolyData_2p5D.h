#pragma once

#include <core/rendered_data/RenderedPolyData.h>


class vtkAlgorithm;
class vtkPolyDataMapper;
class vtkPolyDataNormals;
class vtkProbeFilter;

class PolyDataObject;
class ImageDataObject;


class CORE_API RenderedPolyData_2p5D : public RenderedPolyData
{
public:
    RenderedPolyData_2p5D(PolyDataObject * dataObject);

    reflectionzeug::PropertyGroup * createConfigGroup() override;

protected:
    vtkProperty * createDefaultRenderProperty() const override;

    void finalizePipeline() override;

    void setApplyDEM(bool apply);

private:
    bool m_applyDEM;
    ImageDataObject * m_demData;
    vtkSmartPointer<vtkProbeFilter> m_demProbe;
};
