#pragma once

#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>

#include <core/filters/AbstractSimpleGeoCoordinateTransformFilter.h>


class vtkTransform;
class vtkTransformPolyDataFilter;

class SetCoordinateSystemInformationFilter;


class CORE_API SimplePolyGeoCoordinateTransformFilter :
    public AbstractSimpleGeoCoordinateTransformFilter<vtkPolyDataAlgorithm>
{
public:
    vtkTypeMacro(SimplePolyGeoCoordinateTransformFilter, AbstractSimpleGeoCoordinateTransformFilter);
    static SimplePolyGeoCoordinateTransformFilter * New();


protected:
    SimplePolyGeoCoordinateTransformFilter();
    ~SimplePolyGeoCoordinateTransformFilter() override;

    int RequestDataObject(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int FillInputPortInformation(
        int port, vtkInformation * info) override;

    int FillOutputPortInformation(
        int port, vtkInformation * info) override;

    void SetFilterParameters(
        const vtkVector3d & preTranslate,
        const vtkVector3d & scale,
        const vtkVector3d & postTranslate) override;

private:

    vtkSmartPointer<vtkTransformPolyDataFilter> TransformFilter;
    vtkSmartPointer<vtkTransform> Transform;
    vtkSmartPointer<SetCoordinateSystemInformationFilter> InfoSetter;

private:
    SimplePolyGeoCoordinateTransformFilter(const SimplePolyGeoCoordinateTransformFilter &) = delete;
    void operator=(const SimplePolyGeoCoordinateTransformFilter &) = delete;
};
