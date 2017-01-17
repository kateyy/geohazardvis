#pragma once

#include <vtkDataSetAlgorithm.h>
#include <vtkStdString.h>

#include <core/core_api.h>


/**
 * Uses the valid point mask array generated by vtkProbeFilter to set scalars values of masked
 * points to quiet_NaN.
 *
 * This filter is only useful for floating point scalar data. Also, it won't do anything if the
 * point mask array cannot be found.
 *
 * When processing vtkImageData, this filter is useful in combination with
 * ImageBlankNonFiniteValuesFilter: Non-finite values in the image are blanked using the
 * vtkUniformGrid class and ignored by vtkProbeFilter, which marks affected, invalid points.
 * SetMaskedPointScalarsToNaNFilter finally restores NaNs at relevant points. 
 */
class CORE_API SetMaskedPointScalarsToNaNFilter : public vtkDataSetAlgorithm
{
public:
    vtkTypeMacro(SetMaskedPointScalarsToNaNFilter, vtkDataSetAlgorithm);

    static SetMaskedPointScalarsToNaNFilter * New();

    /**
     * Name of the valid point mask array generated by vtkProbeFilter.
     * Valid points are marked with 1, invalid points with 0 and their attributes are replaced
     * by quiet_NaN (only for float and double values).
     * Defaults to vtkProbeFilter::ValidPointMaskArrayName default value.
     */
    vtkSetMacro(ValidPointMaskArrayName, const vtkStdString &);
    vtkGetMacro(ValidPointMaskArrayName, const vtkStdString &);

    static const vtkStdString & GetDefaultValidPointMaskArrayName();

protected:
    SetMaskedPointScalarsToNaNFilter();
    ~SetMaskedPointScalarsToNaNFilter() override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    vtkStdString ValidPointMaskArrayName;

private:
    SetMaskedPointScalarsToNaNFilter(const SetMaskedPointScalarsToNaNFilter &) = delete;
    void operator=(const SetMaskedPointScalarsToNaNFilter &) = delete;
};