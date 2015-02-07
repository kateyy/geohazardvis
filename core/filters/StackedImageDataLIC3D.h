#pragma once

#include <vtkSimpleImageToImageFilter.h>

#include <core/core_api.h>


class NoiseImageSource;
class vtkRenderWindow;


class CORE_API StackedImageDataLIC3D : public vtkSimpleImageToImageFilter
{
public:
    static StackedImageDataLIC3D * New();
    vtkTypeMacro(StackedImageDataLIC3D, vtkSimpleImageToImageFilter);

protected:
    StackedImageDataLIC3D();
    ~StackedImageDataLIC3D() override;

    void SimpleExecute(vtkImageData * input, vtkImageData * output) override;

private:
    StackedImageDataLIC3D(const StackedImageDataLIC3D &) = delete;
    void operator=(const StackedImageDataLIC3D &) = delete;

    void initialize();

private:
    bool m_isInitialized;

    NoiseImageSource * m_noiseImage;
    vtkRenderWindow * m_glContext;
};