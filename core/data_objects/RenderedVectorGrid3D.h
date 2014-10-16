#pragma once

#include <array>

#include <core/data_objects/RenderedData.h>


class vtkImageData;
class vtkAlgorithmOutput;
class vtkExtractVOI;
class vtkPlaneSource;

class VectorGrid3DDataObject;


class CORE_API RenderedVectorGrid3D : public RenderedData
{
    Q_OBJECT

public:
    RenderedVectorGrid3D(VectorGrid3DDataObject * dataObject);
    ~RenderedVectorGrid3D() override;

    VectorGrid3DDataObject * vectorGrid3DDataObject();
    const VectorGrid3DDataObject * vectorGrid3DDataObject() const;

    reflectionzeug::PropertyGroup * createConfigGroup() override;

    void setSampleRate(int x, int y, int z);
    void sampleRate(int sampleRate[3]);
    vtkImageData * resampledDataSet();
    vtkAlgorithmOutput * resampledOuputPort();

    void setSlicePosition(int axis, int slicePosition);

signals:
    void sampleRateChanged(int x, int y, int z);

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkActor * createActor() override;
    QList<vtkActor *> fetchAttributeActors() override;

    void scalarsForColorMappingChangedEvent() override;
    void gradientForColorMappingChangedEvent() override;
    void visibilityChangedEvent(bool visible) override;

private:
    void updateVisibilities();

private:
    vtkSmartPointer<vtkExtractVOI> m_extractVOI;

    std::array<vtkSmartPointer<vtkExtractVOI>, 3> m_extractSlices;
    std::array<vtkSmartPointer<vtkPlaneSource>, 3> m_slicePlanes;
    std::array<bool, 3> m_slicesEnabled;
    std::array<vtkSmartPointer<vtkActor>, 3> m_sliceActors;
    std::array<vtkSmartPointer<vtkActor>, 3> m_sliceOutlineActors;
};
