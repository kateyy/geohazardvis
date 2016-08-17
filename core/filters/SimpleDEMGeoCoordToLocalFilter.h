#pragma once

#include <vtkImageAlgorithm.h>
#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkImageChangeInformation;
class vtkVector2d;


/** This filter transforms a DEM grid from geographic coordinates (latitude, longitude)
* to a local coordinate system. The applied method only works for regions not larger than a few
* hundred kilometers. */
class CORE_API SimpleDEMGeoCoordToLocalFilter : public vtkImageAlgorithm
{
public:
    vtkTypeMacro(SimpleDEMGeoCoordToLocalFilter, vtkImageAlgorithm);
    static SimpleDEMGeoCoordToLocalFilter * New();

    vtkGetMacro(Enabled, bool);
    vtkSetMacro(Enabled, bool);

protected:
    SimpleDEMGeoCoordToLocalFilter();
    ~SimpleDEMGeoCoordToLocalFilter() override;

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
    SimpleDEMGeoCoordToLocalFilter(const SimpleDEMGeoCoordToLocalFilter &) = delete;
    void operator=(const SimpleDEMGeoCoordToLocalFilter &) = delete;

    void SetParameters(const vtkVector2d & inputGeoCentroid);

private:
    bool Enabled;

    vtkSmartPointer<vtkImageChangeInformation> TranslateFilter;
    vtkSmartPointer<vtkImageChangeInformation> ScaleFilter;
};
