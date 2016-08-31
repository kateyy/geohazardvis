#pragma once

#include <vtkImageAlgorithm.h>

#include <core/core_api.h>


class CORE_API DEMImageNormals : public vtkImageAlgorithm
{
public:
    vtkTypeMacro(DEMImageNormals, vtkImageAlgorithm);
    static DEMImageNormals * New();

    vtkGetMacro(CoordinatesUnitScale, double);
    vtkSetMacro(CoordinatesUnitScale, double);

    vtkGetMacro(ElevationUnitScale, double);
    vtkSetMacro(ElevationUnitScale, double);

protected:
    DEMImageNormals();
    ~DEMImageNormals() override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    double CoordinatesUnitScale;
    double ElevationUnitScale;

private:
    DEMImageNormals(const DEMImageNormals &) = delete;
    void operator=(const DEMImageNormals &) = delete;
};
