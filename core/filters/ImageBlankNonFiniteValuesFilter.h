#pragma once

#include <vtkImageAlgorithm.h>

#include <core/core_api.h>


/**
 * Convert a vtkImageData to its subclass vtkUniformGrid and blank all input points with non-finite
 * scalar values (NaN or positive/negative infinity).
 *
 * As an example, this filter helps to correctly handle invalid values in grid data that is used as
 * source in vtkProbeFilter. The probe filter does not preserve non-finite values in some
 * situations and replaces them with garbage. (See implementation of vtkMath::Max, not properly
 * handling NaNs).
 * With a vtkUniformGrid, invalid values can be identified using the data set's point ghost array.
 */
class CORE_API ImageBlankNonFiniteValuesFilter : public vtkImageAlgorithm
{
public:
    vtkTypeMacro(ImageBlankNonFiniteValuesFilter, vtkImageAlgorithm);
    static ImageBlankNonFiniteValuesFilter * New();

protected:
    ImageBlankNonFiniteValuesFilter();
    ~ImageBlankNonFiniteValuesFilter() override;

    int FillOutputPortInformation(int port, vtkInformation * info) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    ImageBlankNonFiniteValuesFilter(const ImageBlankNonFiniteValuesFilter &) = delete;
    void operator=(const ImageBlankNonFiniteValuesFilter &) = delete;
};
