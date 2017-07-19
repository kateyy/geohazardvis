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

#include <core/utility/memory.h>


template<typename InIt, typename Ty>
InIt findUnique(InIt First, InIt Last, const Ty * value)
{
    auto it = First;
    for (; it != Last; ++it)
    {
        if (it->get() == value)
            break;
    }
    return it;
}

template<typename T>
typename std::vector<std::unique_ptr<T>>::iterator findUnique(std::vector<std::unique_ptr<T>> & vector, const T * value)
{
    return findUnique(vector.begin(), vector.end(), value);
}

template<typename T>
typename std::vector<std::unique_ptr<T>>::const_iterator findUnique(const std::vector<std::unique_ptr<T>> & vector, const T * value)
{
    return findUnique(vector.cbegin(), vector.cend(), value);
}

template<typename T>
bool containsUnique(const std::vector<std::unique_ptr<T>> & vector, const T * value)
{
    return findUnique(vector, value) != vector.end();
}
