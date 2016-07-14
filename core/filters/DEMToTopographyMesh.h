#pragma once

#include <vtkAlgorithm.h>
#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include <core/core_api.h>
#include <core/utility/DataExtent.h>


class vtkImageData;
class vtkImageShiftScale;
class vtkPolyData;
class vtkTransform;
class vtkTransformPolyDataFilter;
class vtkWarpScalar;


/** Apply elevations from a Digital Elevation Model to a polygonal mesh, 
generating a topography representation.

This filter has two inputs:
    - (0) the DEM stored as vtkImageData
    - (1) a polygonal data set that will be transformed according to the DEM's elevations.

The input mesh template is assumed to be centered on the origin (for X,Y). It's current radius is
computed by searching for the longest distance from any point of the data set to the origin (0, 0, z).
The mesh's elevations (z coordinates) are ignored and replaced by DEM elevations.

The transformation process is as follows:
    - Shift and scale the mesh template on the DEM
    - Apply DEM's values to mesh point elevations (output port 1)
    - Center the output mesh on the origin (x=0, y=0) (output port 0)
*/
class CORE_API DEMToTopographyMesh : public vtkAlgorithm
{
public:
    vtkTypeMacro(DEMToTopographyMesh, vtkAlgorithm);
    static DEMToTopographyMesh * New();

    /** Set the input elevation model (image data). This sets the input for port 0 */
    void SetInputDEM(vtkImageData * dem);

    /** Set the input mesh template. This sets the input for port 1 */
    void SetInputMeshTemplate(vtkPolyData * meshTemplate);

    /** @return transformed topography mesh centered around the origin */
    vtkPolyData * GetOutputTopography();
    vtkAlgorithmOutput * GetTopographyOutputPort();
    /** @return transformed topography mesh, localed on (local) DEM coordinates */
    vtkPolyData * GetOutputTopoMeshOnDEM();
    vtkAlgorithmOutput * GetTopoMeshOnDEMOutputPort();

    vtkGetMacro(ElevationScaleFactor, double);
    vtkSetMacro(ElevationScaleFactor, double);

    vtkGetMacro(TopographyRadius, double);
    vtkSetMacro(TopographyRadius, double);

    const vtkVector2d & GetTopographyShiftXY();
    vtkSetMacro(TopographyShiftXY, vtkVector2d);
    vtkSetVector2Macro(TopographyShiftXY, double);


    /** Set the topography position to be centered on the DEM's coordinates.
    * This requires valid inputs on port 0 and 1. */
    bool CenterTopoMesh();

    /** Set the topography radius to the minimum of the DEM's x and y size.
    * This requires valid inputs on port 0 and 1. */
    bool MatchTopoMeshRadius();

    /** Center the topography mesh in the DEM and select the maximum radius so that all mesh points
    * fall into the DEM area.*/
    bool SetParametersToMatching();

    /** Based on the current topography extent and TopographyRadius value, compute valid shift 
    * ranges for the topography mesh in x and y direction.
    * A mesh position is valid, if all mesh points lie within the DEM area.
    * @returns a valid (non-empty) range only if a valid input topography is set and the radius
    *   value is valid (smaller than the topography's x and y size) */
    DataExtent<double, 2> ComputeValidShiftRange(bool * isValid = nullptr);
    /** Based on the current topography extent and TopographyShiftXY values, compute the maximum
    * mesh radius so that all mesh points lie inside the DEM area.
    * This does not modify any parameters, but requires valid input data.
    * @returns a valid (non-empty) range only if a valid input topography is set and the current
    *   topography shift is valid (mesh center inside the input DEM area). */
    DataExtent<double, 1> ComputeValidRadiusRange(bool * isValid = nullptr);

protected:
    DEMToTopographyMesh();
    ~DEMToTopographyMesh() override;

    int FillOutputPortInformation(int port, vtkInformation * info) override;
    int FillInputPortInformation(int port, vtkInformation * info) override;

    int ProcessRequest(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    virtual int RequestDataObject(vtkInformation * request,
        vtkInformationVector** inputVector,
        vtkInformationVector* outputVector);

    virtual int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector);

    virtual int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector);

    static double ComputeCenteredMeshRadius(vtkPolyData & mesh);

private:
    void SetupPipeline();

    vtkImageData * CheckUpdateInputDEM();

private:
    vtkSmartPointer<vtkImageShiftScale> DEMScaleElevationFilter;
    vtkSmartPointer<vtkTransform> MeshTransform;
    vtkSmartPointer<vtkTransformPolyDataFilter> MeshTransformFilter;
    vtkSmartPointer<vtkWarpScalar> WarpElevation;
    vtkSmartPointer<vtkTransform> CenterOutputMeshTransform;
    vtkSmartPointer<vtkTransformPolyDataFilter> CenterOutputMeshFilter;

    double ElevationScaleFactor;
    double TopographyRadius;
    vtkVector2d TopographyShiftXY;

private:
    DEMToTopographyMesh(const DEMToTopographyMesh &) = delete;
    void operator=(const DEMToTopographyMesh &) = delete;
};
