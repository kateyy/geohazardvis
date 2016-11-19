#pragma once

#include <vtkStdString.h>

#include <core/CoordinateSystems.h>
#include <core/utility/DataExtent_fwd.h>


class vtkInformation;
class vtkInformationVector;
class vtkImageAlgorithm;
class vtkPolyDataAlgorithm;
class vtkVector3d;


/** This filter implements a set of coordinate system and coordinate unit transformations.
*
* The following transformations are provided:
*   * Geographic (longitude/latitude) <-> local (metric)
*     A reference point (geographic and local) must be specified for this transformation
*     The method applied here only works for regions not larger than a few hundred kilometers.
*   * Global (metric) -> local (metric)
*     The data set is centered around its local reference point.
*   * Metric unit conversions
*     Input/output metric units can be specified/requested with an arbitrary SI unit prefix
*     (always in *meters).
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

    static bool isConversionSupported(CoordinateSystemType from, CoordinateSystemType to);


protected:
    AbstractSimpleGeoCoordinateTransformFilter();
    ~AbstractSimpleGeoCoordinateTransformFilter() override;

    int ProcessRequest(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    virtual int RequestDataObject(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector);

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestDataInternal(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector);

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    virtual void SetFilterParameters(
        const vtkVector3d & preTranslate,
        const vtkVector3d & scale,
        const vtkVector3d & postTranslate) = 0;

private:
    void ComputeFilterParameters(const DataBounds * inBounds = nullptr);

protected:
    ReferencedCoordinateSystemSpecification SourceCoordinateSystem;
    ReferencedCoordinateSystemSpecification TargetCoordinateSystem;

private:
    CoordinateSystemType TargetCoordinateSystemType;
    vtkStdString TargetMetricUnit;

    bool RequestFilterParametersWithBounds;

private:
    AbstractSimpleGeoCoordinateTransformFilter(const AbstractSimpleGeoCoordinateTransformFilter &) = delete;
    void operator=(const AbstractSimpleGeoCoordinateTransformFilter &) = delete;
};
