#pragma once

#include <core/core_api.h>


class QString;


namespace mathhelper
{

/** @return -1 if value < 0, 1 else */
template <typename T> char sgn(const T & value);

bool CORE_API circleLineIntersection(double radius, double P0[2], double P1[2], double intersection1[2], double intersection2[2]);

double CORE_API scaleFactorForMetricUnits(const QString & from, const QString & to);
bool CORE_API isValidMetricUnit(const QString & unit);

}


#include "mathhelper.hpp"
