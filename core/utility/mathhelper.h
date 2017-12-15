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


class QString;
class vtkVector3d;


namespace mathhelper
{

/** @return -1 if value < 0, 1 else */
template <typename T> char sgn(const T & value);

bool CORE_API circleLineIntersection(double radius, double P0[2], double P1[2], double intersection1[2], double intersection2[2]);

double CORE_API scaleFactorForMetricUnits(const QString & from, const QString & to);
bool CORE_API isValidMetricUnit(const QString & unit);

/**
 * Compute incidence angle (alpha, in degrees) and satellite heading (theta, in degrees)
 * from the line-of-sight vector.
 * @param losVector Input line-of-sight vector. Will be normalized.
 * @param incidenceAngleDegrees Output incidence angle ("alpha") in degrees
 * @param satelliteHeadingDegrees Output satellite heading ("theta") in degrees
 */
void CORE_API losVectorToSatelliteAngles(const vtkVector3d & losVector,
    double & incidenceAngleDegrees, double & satelliteHeadingDegrees);

/**
 * Compute the line-of-sight vector based on incidence angle and satellite heading, both in
 * degrees.
 */
vtkVector3d CORE_API satelliteAnglesToLOSVector(double incidenceAngleDegrees, double satelliteHeadingDegrees);

}


#include "mathhelper.hpp"
