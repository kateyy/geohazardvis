#pragma once

#include <array>

#include <core/rendered_data/RenderedData3D.h>


class vtkAlgorithmOutput;
class vtkArrayCalculator;
class vtkAssignAttribute;
class vtkExtractVOI;
class vtkImageData;
class vtkImageDataLIC2D;
class vtkImagePlaneWidget;
class vtkProperty;
class vtkRenderWindow;
class vtkRenderWindowInteractor;

class NoiseImageSource;
class VectorGrid3DDataObject;


class CORE_API RenderedVectorGrid3D : public RenderedData3D
{
    Q_OBJECT

public:
    RenderedVectorGrid3D(VectorGrid3DDataObject * dataObject);
    ~RenderedVectorGrid3D() override;

    /** the interactor needs to be set in order to use the image plane widgets */
    void setRenderWindowInteractor(vtkRenderWindowInteractor * interactor);

    VectorGrid3DDataObject * vectorGrid3DDataObject();
    const VectorGrid3DDataObject * vectorGrid3DDataObject() const;

    reflectionzeug::PropertyGroup * createConfigGroup() override;

    void setSampleRate(int x, int y, int z);
    void sampleRate(int sampleRate[3]);
    vtkImageData * resampledDataSet();
    vtkAlgorithmOutput * resampledOuputPort();

    int slicePosition(int axis);
    void setSlicePosition(int axis, int slicePosition);
    void setLic2DVectorScaleFactor(float f);

    enum class ColorMode
    {
        UserDefined,
        ScalarMapping,
        LIC
    };
    ColorMode colorMode() const;
    void setColorMode(ColorMode mode);

signals:
    void sampleRateChanged(int x, int y, int z);

protected:
    vtkSmartPointer<vtkProp3DCollection> fetchViewProps3D() override;

    void scalarsForColorMappingChangedEvent() override;
    void colorMappingGradientChangedEvent() override;
    void visibilityChangedEvent(bool visible) override;

    /** work around missing update after LIC image changes */
    void forceLICUpdate(int axis);
    /** after updating the reslice inputs: set slice positions to the previous values */
    void resetSlicePositions();

private:
    void updateVisibilities();

private:
    friend class VectorField3DLIC2DPlanes;

    bool m_isInitialized;

    // for vector mapping
    vtkSmartPointer<vtkExtractVOI> m_extractVOI;


    // for color mapping / LIC2D planes

    ColorMode m_colorMode;

    vtkSmartPointer<vtkProperty> m_texturePlaneProperty;
    std::array<vtkSmartPointer<vtkImagePlaneWidget>, 3> m_planeWidgets;

    std::array<bool, 3> m_slicesEnabled;
    std::array<int, 3> m_slicePositions;

    vtkSmartPointer<NoiseImageSource> m_noiseImage;
    float m_lic2DVectorScaleFactor;
    std::array<vtkSmartPointer<vtkArrayCalculator>, 3> m_lic2DVectorScale;
    std::array<vtkSmartPointer<vtkImageDataLIC2D>, 3> m_lic2D;
    vtkSmartPointer<vtkRenderWindow> m_glContext;
};
