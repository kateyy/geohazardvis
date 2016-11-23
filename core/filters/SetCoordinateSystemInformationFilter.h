#pragma once

#include <vtkPassInputTypeAlgorithm.h>

#include <core/CoordinateSystems.h>


/** This filter sets the coordinate system information on the output, without actually transforming
* the coordinates.*/
class CORE_API SetCoordinateSystemInformationFilter : public vtkPassInputTypeAlgorithm
{
public:
    vtkTypeMacro(SetCoordinateSystemInformationFilter, vtkPassInputTypeAlgorithm);

    static SetCoordinateSystemInformationFilter * New();
    
    vtkSetMacro(CoordinateSystemSpec, ReferencedCoordinateSystemSpecification);
    vtkGetMacro(CoordinateSystemSpec, const ReferencedCoordinateSystemSpecification &);

protected:
    SetCoordinateSystemInformationFilter();
    ~SetCoordinateSystemInformationFilter() override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    ReferencedCoordinateSystemSpecification CoordinateSystemSpec;

private:
    SetCoordinateSystemInformationFilter(const SetCoordinateSystemInformationFilter &) = delete;
    void operator=(const SetCoordinateSystemInformationFilter &) = delete;
};
