/*
 * GeohazardVis plug-in: pCDM Modeling
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

#include "DataExtent_print.h"


template<typename T, size_t Dimensions>
std::ostream & operator<<(std::ostream & os, const DataExtent<T, Dimensions> & dataExtent)
{
    char axisChar = Dimensions <= 3u ? 'x' : '0';

    os << "DataExtent (" << Dimensions << " dimensions)\n";
    for (size_t d = 0u; d < Dimensions; ++d)
    {
        os << "    " << static_cast<char>(axisChar + d)
            << ": " << dataExtent[2u * d] << "; " << dataExtent[2u * d + 1u] << "\n";
    }

    return os;
}
