#pragma once

#include <core/core_api.h>


class vtkCamera;


namespace TerrainCamera
{
    /** get/set azimuth (in degrees) for terrain navigations
      * up vector must be (0, 0, 1) 
      * azimuth is zero for a view vector of (1, 0, z) */
    double CORE_API getAzimuth(vtkCamera & camera);
    void CORE_API setAzimuth(vtkCamera & camera, double azimuth);

    double CORE_API getVerticalElevation(vtkCamera & camera);
    void CORE_API setVerticalElevation(vtkCamera & camera, double elevation);

    /** Set the distance from camera position to focal point by moving the camera position.
      * @note vtkCamera::setDistance moves the camera focal point instead. So use the vtkCamera's function
      * or this one here, based on your purpose. */
    void CORE_API setDistanceFromFocalPoint(vtkCamera & camera, double distance);
}
