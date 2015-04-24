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
}
