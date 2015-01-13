#pragma once

#include <array>

#include <core/rendered_data/RenderedData3D.h>


class vtkAlgorithmOutput;
class vtkArrayCalculator;
class vtkExtractVOI;
class vtkImageData;
class vtkImageDataLIC2D;
class vtkImageMapToColors;
class vtkImageProperty;
class vtkImageSlice;
class vtkImageSliceMapper;
class vtkRenderWindow;

class VectorGrid3DDataObject;


class CORE_API RenderedVectorGrid3D : public RenderedData3D
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

private:
    void updateVisibilities();

private:
    friend class VectorField3DLIC2DPlanes;

    bool m_isInitialized;

    vtkSmartPointer<vtkExtractVOI> m_extractVOI;

    ColorMode m_colorMode;
    vtkSmartPointer<vtkImageData> m_nullImage;

    std::array<bool, 3> m_slicesEnabled;
    std::array<vtkSmartPointer<vtkImageSliceMapper>, 3> m_sliceMappers;
    std::array<vtkSmartPointer<vtkImageSlice>, 3> m_slices;
    std::array<int, 3> m_slicePositions;
    vtkSmartPointer<vtkImageProperty> m_imgProperty;

    float m_lic2DVectorScaleFactor;
    vtkSmartPointer<vtkArrayCalculator> m_lic2DVectorScale;
    std::array<vtkSmartPointer<vtkExtractVOI>, 3> m_lic2DVOI;
    std::array<vtkSmartPointer<vtkImageDataLIC2D>, 3> m_lic2D;
    vtkSmartPointer<vtkRenderWindow> m_glContext;
};
