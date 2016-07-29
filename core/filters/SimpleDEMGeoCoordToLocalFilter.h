#pragma once

#include <vtkImageAlgorithm.h>
#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include <core/core_api.h>


class vtkImageChangeInformation;


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

    vtkGetMacro(UseNorthWestAsOrigin, bool);
    vtkSetMacro(UseNorthWestAsOrigin, bool);

    const vtkVector2d & GetGeoOrigin();
    vtkSetMacro(GeoOrigin, vtkVector2d);

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

    void SetParameters(const vtkVector3d & inputGeoNorthWest);

private:
    bool Enabled;
    bool UseNorthWestAsOrigin;
    vtkVector2d GeoOrigin;

    vtkSmartPointer<vtkImageChangeInformation> TranslateFilter;
    vtkSmartPointer<vtkImageChangeInformation> ScaleFilter;
};
