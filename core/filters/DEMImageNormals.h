#pragma once

#include <vtkThreadedImageAlgorithm.h>

#include <core/core_api.h>


class CORE_API DEMImageNormals : public vtkThreadedImageAlgorithm
{
public:
    vtkTypeMacro(DEMImageNormals, vtkThreadedImageAlgorithm);
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
    
    void CopyAttributeData(vtkImageData * in, vtkImageData * out,
        vtkInformationVector ** inputVector) override;

    void ThreadedRequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector,
        vtkImageData *** inData, vtkImageData ** outData,
        int outExt[6], int id) override;

private:
    double CoordinatesUnitScale;
    double ElevationUnitScale;

private:
    DEMImageNormals(const DEMImageNormals &) = delete;
    void operator=(const DEMImageNormals &) = delete;
};
