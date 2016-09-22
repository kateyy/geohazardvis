#pragma once

#include <vtkImageAlgorithm.h>
#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkImageChangeInformation;

struct ReferencedCoordinateSystemSpecification;


/** This filter transforms an image between from geographic coordinates (latitude, longitude)
* and a local coordinate system. The applied method only works for regions not larger than a few
* hundred kilometers.
* This algorithm only covers a simple, specific use case, thus the specification defined by
* CoordinateSystemSpecification is not used here. However, the coordinate system of input data
* needs to be defined using the information set by ReferencedCoordinateSystemSpecification. Only
* CoordinateSystemType::geographic and metricLocal is supported as input coordinate systems.
* geographicSystem and globalMetricSystem parameters are ignored.
*/
class CORE_API SimpleImageGeoCoordinateTransformFilter : public vtkImageAlgorithm
{
public:
    vtkTypeMacro(SimpleImageGeoCoordinateTransformFilter, vtkImageAlgorithm);
    static SimpleImageGeoCoordinateTransformFilter * New();

    enum CoordinateSystem
    {
        GLOBAL_GEOGRAPHIC,
        LOCAL_METRIC,
        UNSPECIFIED
    };

    /** Target coordinate system to transform to.
    * This is LOCAL_METRIC by default. */
    vtkSetClampMacro(TargetCoordinateSystem, CoordinateSystem, GLOBAL_GEOGRAPHIC, LOCAL_METRIC)
    vtkGetMacro(TargetCoordinateSystem, CoordinateSystem);


protected:
    SimpleImageGeoCoordinateTransformFilter();
    ~SimpleImageGeoCoordinateTransformFilter() override;

    int ProcessRequest(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    virtual int RequestDataObject(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector);

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    void SetFilterParameters(const ReferencedCoordinateSystemSpecification & targetSpec);

private:
    CoordinateSystem SourceCoordinateSystem;
    CoordinateSystem TargetCoordinateSystem;

    vtkSmartPointer<vtkImageChangeInformation> Step1;
    vtkSmartPointer<vtkImageChangeInformation> Step2;

private:
    SimpleImageGeoCoordinateTransformFilter(const SimpleImageGeoCoordinateTransformFilter &) = delete;
    void operator=(const SimpleImageGeoCoordinateTransformFilter &) = delete;
};
