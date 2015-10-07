#pragma once

#include <array>

#include <core/rendered_data/RenderedData3D.h>


class vtkAlgorithm;
class vtkAlgorithmOutput;
class vtkExtractVOI;
class vtkImageData;
class vtkLookupTable;
class vtkRenderWindowInteractor;

class ImagePlaneWidget;
class VectorGrid3DDataObject;


class CORE_API RenderedVectorGrid3D : public RenderedData3D
{
    Q_OBJECT

public:
    RenderedVectorGrid3D(VectorGrid3DDataObject & dataObject);

    /** the interactor needs to be set in order to use the image plane widgets */
    void setRenderWindowInteractor(vtkRenderWindowInteractor * interactor);

    VectorGrid3DDataObject & vectorGrid3DDataObject();
    const VectorGrid3DDataObject & vectorGrid3DDataObject() const;

    reflectionzeug::PropertyGroup * createConfigGroup() override;

    void setSampleRate(int x, int y, int z);
    void sampleRate(int sampleRate[3]);
    vtkImageData * resampledDataSet();
    vtkAlgorithmOutput * resampledOuputPort();

    int slicePosition(int axis);
    void setSlicePosition(int axis, int slicePosition);

    int numberOfColorMappingInputs() const override;
    vtkAlgorithmOutput * colorMappingInput(int connection = 0) override;

signals:
    void sampleRateChanged(int x, int y, int z);

protected:
    vtkSmartPointer<vtkProp3DCollection> fetchViewProps3D() override;

    void scalarsForColorMappingChangedEvent() override;
    void colorMappingGradientChangedEvent() override;
    void visibilityChangedEvent(bool visible) override;

    void updatePlaneLUT();

private:
    void updateVisibilities();

private:
    bool m_isInitialized;

    // for vector mapping
    vtkSmartPointer<vtkExtractVOI> m_extractVOI;

    // for color mapping / LIC2D planes

    std::array<vtkSmartPointer<ImagePlaneWidget>, 3> m_planeWidgets;
    std::array<vtkSmartPointer<vtkAlgorithm>, 3> m_colorMappingInputs;
    vtkSmartPointer<vtkLookupTable> m_blackWhiteLUT;

    std::array<bool, 3> m_slicesEnabled;
    std::array<int, 3> m_storedSliceIndexes;
};
