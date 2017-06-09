#pragma once

#include <vtkImageAlgorithm.h>
#include <vtkSmartPointer.h>

#include <core/filters/AbstractSimpleGeoCoordinateTransformFilter.h>


class vtkImageChangeInformation;

class SetCoordinateSystemInformationFilter;


class CORE_API SimpleImageGeoCoordinateTransformFilter :
    public AbstractSimpleGeoCoordinateTransformFilter<vtkImageAlgorithm>
{
public:
    vtkTypeMacro(SimpleImageGeoCoordinateTransformFilter,
        AbstractSimpleGeoCoordinateTransformFilter<vtkImageAlgorithm>);
    static SimpleImageGeoCoordinateTransformFilter * New();

protected:
    SimpleImageGeoCoordinateTransformFilter();
    ~SimpleImageGeoCoordinateTransformFilter() override;

    int RequestDataObject(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    void SetFilterParameters(
        const vtkVector3d & preTranslate,
        const vtkVector3d & scale,
        const vtkVector3d & postTranslate) override;

private:
    vtkSmartPointer<vtkImageChangeInformation> Step1;
    vtkSmartPointer<vtkImageChangeInformation> Step2;
    vtkSmartPointer<SetCoordinateSystemInformationFilter> InfoSetter;

private:
    SimpleImageGeoCoordinateTransformFilter(const SimpleImageGeoCoordinateTransformFilter &) = delete;
    void operator=(const SimpleImageGeoCoordinateTransformFilter &) = delete;
};
