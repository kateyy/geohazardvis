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

#include "BinaryFile.h"


template<typename T>
bool BinaryFile::write(const std::vector<T> & data)
{
    return write(data.data(), data.size() * sizeof(T));
}

template<typename T>
bool BinaryFile::writeStruct(const T & plainDataStruct)
{
    return write(&plainDataStruct, sizeof(T));
}

template<typename T>
bool BinaryFile::read(size_t numValues, std::vector<T> & data)
{
    data.resize(numValues);

    const auto numBytesRead = read(numValues * sizeof(T), data.data());

    const auto numValuesRead = numBytesRead / sizeof(T);

    if (numValuesRead != numValues)
    {
        data.resize(numValuesRead);
        return false;
    }

    return true;
}

template<typename T>
bool BinaryFile::readStruct(T & plainDataStruct)
{
    const auto numBytesRead = read(sizeof(T), &plainDataStruct);

    return numBytesRead == sizeof(T);
}
