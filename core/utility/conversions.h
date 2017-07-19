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

#include <array>

#include <QString>

#include <vtkVector.h>


template<typename ValueType, size_t Size>
QString arrayToString(const std::array<ValueType, Size> & a,
    const QString & separator = " ", const QString & prefix = "", const QString & suffix = "");

template<typename ValueType, size_t Size>
void stringToArray(const QString & s, std::array<ValueType, Size> & resultArray);

template<typename ValueType, size_t Size>
std::array<ValueType, Size> stringToArray(const QString & s);

template<typename ValueType, int Size>
vtkVector<ValueType, Size> stringToVector(const QString & s);

template<typename ValueType, int Size>
void stringToVector(const QString & s, vtkVector<ValueType, Size> & resultVector);

template<typename ValueType>
void stringToVector2(const QString & s, vtkVector2<ValueType> & resultVector);

template<typename ValueType>
vtkVector2<ValueType> stringToVector2(const QString & s);

template<typename ValueType>
void stringToVector3(const QString & s, vtkVector3<ValueType> & resultVector);

template<typename ValueType>
vtkVector3<ValueType> stringToVector3(const QString & s);

template<typename ValueType>
QString vector3ToString(const vtkVector3<ValueType> & v);

template<typename ValueType, int Size>
QString vectorToString(const vtkVector<ValueType, Size> & vector,
    const QString & separator = " ", const QString & prefix = "", const QString & suffix = "");


#include "conversions.hpp"
