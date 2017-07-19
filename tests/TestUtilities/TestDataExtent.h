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

#include <iomanip>

#include <core/utility/DataExtent_print.h>


/** Workaround for gtest not selecting the expected PrintTo or operator<<
 * (Cannot work for template classes)
 * https://stackoverflow.com/questions/25146997/teach-google-test-how-to-print-eigen-matrix
 */
class tDataBounds : public DataBounds
{
public:
    using DataBounds::DataBounds;
};

class tImageExtent : public ImageExtent
{
public:
    using ImageExtent::ImageExtent;
};


inline void PrintTo(const tDataBounds & bounds, std::ostream * os)
{
    *os  << std::setprecision(17) << bounds;
}

inline void PrintTo(const tImageExtent & extent, std::ostream * os)
{
    *os << extent;
}
