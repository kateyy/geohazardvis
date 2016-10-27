#pragma once

#include <vtkStdString.h>

#include <core/CoordinateSystems.h>


class vtkInformation;
class vtkInformationVector;
class vtkImageAlgorithm;
class vtkPolyDataAlgorithm;
class vtkVector3d;


/** This filter transforms a data set between geographic coordinates (latitude, longitude)
* and a local coordinate system. The applied method only works for regions not larger than a few
* hundred kilometers.
* Further, it allows to transform the unit of measurement for metric coordinate (global or local).
* Transformation to or from global metric coordinates (e.g., UTM) is not supported.
* When transforming between geographic and local coordinates, reference points in the input
* data set must be set.
*/
template<typename Superclass_t>
class AbstractSimpleGeoCoordinateTransformFilter : public Superclass_t
{
public:
    vtkAbstractTypeMacro(AbstractSimpleGeoCoordinateTransformFilter, Superclass_t);

    /** Target coordinate system to transform to.
      * This is CoordinateSystemType::metricLocal by default. */
    void SetTargetCoordinateSystemType(CoordinateSystemType type);
    CoordinateSystemType GetTargetCoordinateSystemType() const;

    /** Set the unit of measurement that is used for metric target systems. This is km by default */
    vtkSetMacro(TargetMetricUnit, const vtkStdString &);
    vtkGetMacro(TargetMetricUnit, const vtkStdString &);


protected:
    AbstractSimpleGeoCoordinateTransformFilter();
    ~AbstractSimpleGeoCoordinateTransformFilter() override;

    int ProcessRequest(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    virtual int RequestDataObject(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector);

    virtual int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector);

    virtual int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector);

    virtual void SetFilterParameters(
        const vtkVector3d & preTranslate,
        const vtkVector3d & scale,
        const vtkVector3d & postTranslate) = 0;

private:
    void ComputeFilterParameters();

protected:
    ReferencedCoordinateSystemSpecification SourceCoordinateSystem;
    ReferencedCoordinateSystemSpecification TargetCoordinateSystem;

private:
    CoordinateSystemType TargetCoordinateSystemType;
    vtkStdString TargetMetricUnit;

private:
    AbstractSimpleGeoCoordinateTransformFilter(const AbstractSimpleGeoCoordinateTransformFilter &) = delete;
    void operator=(const AbstractSimpleGeoCoordinateTransformFilter &) = delete;
};
