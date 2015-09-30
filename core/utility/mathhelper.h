#pragma once

#include <core/core_api.h>


namespace mathhelper
{

/** @return -1 if value < 0, 1 else */
template <typename T> char sgn(const T & value);

bool CORE_API circleLineIntersection(double radius, double P0[2], double P1[2], double intersection1[2], double intersection2[2]);

}


#include "mathhelper.hpp"
