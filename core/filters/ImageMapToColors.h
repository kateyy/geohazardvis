#pragma once

#include <vtkImageMapToColors.h>

#include <core/core_api.h>


/**
 * Extension/fix of VTK's vtkImageMapToColors.
 *
 * When input image data bounds (origin, spacing) are modified but not propagated in the
 * information update pass, vtkImageMapToColors does not update the structure of its output.
 * Output image bounds and spacing remain the same, thus are wrong.
 * This implementation just adds a CopyStructure call in the data request pass.
 *
 * NOTE: vtkImageMapToColors "consumes" its input scalars, thus it does not allow to pass-through
 * the original data. At some places that would be required, so an option to pass through the input
 * would be a useful extension. (see RenderedImageData.cpp)
 */
class CORE_API ImageMapToColors : public vtkImageMapToColors
{
public:
    vtkTypeMacro(ImageMapToColors, vtkImageMapToColors);

    static ImageMapToColors * New();

protected:
    ImageMapToColors();
    ~ImageMapToColors() override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    ImageMapToColors(const ImageMapToColors &) = delete;
    void operator=(const ImageMapToColors &) = delete;
};
