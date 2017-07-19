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

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

#include <vtkVector.h>


template<typename T, int Size>
vtkVector<T, Size> & operator+=(vtkVector<T, Size> & lhs, const vtkVector<T, Size> & rhs);

template<typename T, int Size>
vtkVector<T, Size> operator+(vtkVector<T, Size> lhs, const vtkVector<T, Size> & rhs);

template<typename T, int Size>
vtkVector<T, Size> & operator-=(vtkVector<T, Size> & lhs, const vtkVector<T, Size> & rhs);

template<typename T, int Size>
vtkVector<T, Size> operator-(vtkVector<T, Size> lhs, const vtkVector<T, Size> & rhs);

template<typename T, int Size>
vtkVector<T, Size> & operator*=(vtkVector<T, Size> & lhs, const vtkVector<T, Size> & rhs);

template<typename T, int Size>
vtkVector<T, Size> operator*(vtkVector<T, Size> lhs, const vtkVector<T, Size> & rhs);

template<typename T, int Size>
vtkVector<T, Size> & operator/=(vtkVector<T, Size> & lhs, const vtkVector<T, Size> & rhs);

template<typename T, int Size>
vtkVector<T, Size> operator/(vtkVector<T, Size> lhs, const vtkVector<T, Size> & rhs);


template<typename T, int Size>
vtkVector<T, Size> & operator+=(vtkVector<T, Size> & lhs, const T & scalar);

template<typename T, int Size>
vtkVector<T, Size> operator+(vtkVector<T, Size> lhs, const T & scalar);

template<typename T, int Size>
vtkVector<T, Size> & operator-=(vtkVector<T, Size> & lhs, const T & scalar);

template<typename T, int Size>
vtkVector<T, Size> operator-(vtkVector<T, Size> lhs, const T & scalar);

template<typename T, int Size>
vtkVector<T, Size> & operator*=(vtkVector<T, Size> & lhs, const T & scalar);

template<typename T, int Size>
vtkVector<T, Size> operator*(vtkVector<T, Size> lhs, const T & scalar);

template<typename T, int Size>
vtkVector<T, Size> & operator/=(vtkVector<T, Size> & lhs, const T & scalar);

template<typename T, int Size>
vtkVector<T, Size> operator/(vtkVector<T, Size> lhs, const T & scalar);


template<typename T, int Size>
vtkVector<T, Size> operator+(const T & scalar, vtkVector<T, Size> rhs);

template<typename T, int Size>
vtkVector<T, Size> operator-(const T & scalar, vtkVector<T, Size> rhs);

template<typename T, int Size>
vtkVector<T, Size> operator*(const T & scalar, vtkVector<T, Size> rhs);

template<typename T, int Size>
vtkVector<T, Size> operator/(const T & scalar, vtkVector<T, Size> rhs);

template<typename T, int Size>
vtkVector<T, Size> operator-(vtkVector<T, Size> rhs);


template<int SizeOut, int SizeIn, typename T>
typename std::enable_if<(SizeOut < SizeIn), vtkVector<T, SizeOut>>::type
convertTo(const vtkVector<T, SizeIn> & other);

template<int SizeOut, int SizeIn, typename T>
typename std::enable_if<(SizeOut == SizeIn), vtkVector<T, SizeOut>>::type
convertTo(vtkVector<T, SizeIn> other, const T initValue = T());

template<int SizeOut, int SizeIn, typename T>
typename std::enable_if<(SizeOut > SizeIn), vtkVector<T, SizeOut>>::type
convertTo(const vtkVector<T, SizeIn> & other, const T initValue = T());

template<typename TOut, typename TIn, int Size>
typename std::enable_if<std::is_same<TOut, TIn>::value, vtkVector<TOut, Size>>::type
convertTo(vtkVector<TIn, Size> other);

template<typename TOut, typename TIn, int Size>
typename std::enable_if<!std::is_same<TOut, TIn>::value, vtkVector<TOut, Size>>::type
convertTo(const vtkVector<TIn, Size> & other);

template<typename T, int Size>
vtkVector<T, Size> abs(const vtkVector<T, Size> & other);

/** @return the value of the component with the smallest value */
template<typename T, int Size>
T minComponent(const vtkVector<T, Size> & vector);

/** @return the value of the component with the highest value */
template<typename T, int Size>
T maxComponent(const vtkVector<T, Size> & vector);

/** Component-wise min */
template<typename T, int Size>
vtkVector<T, Size> min(const vtkVector<T, Size> & lhs, const vtkVector<T, Size> & rhs);

/** Component-wise max */
template<typename T, int Size>
vtkVector<T, Size> max(const vtkVector<T, Size> & lhs, const vtkVector<T, Size> & rhs);


template<typename T, int Size>
typename std::enable_if<std::numeric_limits<T>::has_quiet_NaN, void>::type
uninitializeVector(vtkVector<T, Size> & vector);

template<typename T, int Size>
typename std::enable_if<std::numeric_limits<T>::has_quiet_NaN, vtkVector<T, Size>>::type
uninitializedVector();

template<typename T, int Size>
typename std::enable_if<std::numeric_limits<T>::has_quiet_NaN, bool>::type
isVectorInitialized(const vtkVector<T, Size> & vector);


#include "vtkvectorhelper.hpp"
