#pragma once

#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkTransform;
class vtkTransformPolyDataFilter;

struct ReferencedCoordinateSystemSpecification;
class SetCoordinateSystemInformationFilter;


/** This filter transforms a poly data set between from geographic coordinates (latitude, longitude)
* and a local coordinate system. The applied method only works for regions not larger than a few
* hundred kilometers.
* This algorithm only covers a simple, specific use case, thus the specification defined by
* CoordinateSystemSpecification is not used here. However, the coordinate system of input data
* needs to be defined using the information set by ReferencedCoordinateSystemSpecification. Only
* CoordinateSystemType::geographic and metricLocal is supported as input coordinate systems.
* geographicSystem and globalMetricSystem parameters are ignored.
*/
class CORE_API SimplePolyGeoCoordinateTransformFilter : public vtkPolyDataAlgorithm
{
public:
    vtkTypeMacro(SimplePolyGeoCoordinateTransformFilter, vtkPolyDataAlgorithm);
    static SimplePolyGeoCoordinateTransformFilter * New();

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
    SimplePolyGeoCoordinateTransformFilter();
    ~SimplePolyGeoCoordinateTransformFilter() override;

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

    vtkSmartPointer<vtkTransformPolyDataFilter> TransformFilter;
    vtkSmartPointer<vtkTransform> Transform;
    vtkSmartPointer<SetCoordinateSystemInformationFilter> InfoSetter;

private:
    SimplePolyGeoCoordinateTransformFilter(const SimplePolyGeoCoordinateTransformFilter &) = delete;
    void operator=(const SimplePolyGeoCoordinateTransformFilter &) = delete;
};
