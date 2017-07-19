/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
