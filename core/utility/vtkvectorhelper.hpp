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

#include "vtkvectorhelper.h"


template<typename T, int Size>
vtkVector<T, Size> & operator+=(vtkVector<T, Size> & lhs, const vtkVector<T, Size> & rhs)
{
    for (int i = 0; i < Size; ++i)
    {
        lhs[i] += rhs[i];
    }

    return lhs;
}

template<typename T, int Size>
vtkVector<T, Size> operator+(vtkVector<T, Size> lhs, const vtkVector<T, Size> & rhs)
{
    lhs += rhs;

    return lhs;
}

template<typename T, int Size>
vtkVector<T, Size> & operator-=(vtkVector<T, Size> & lhs, const vtkVector<T, Size> & rhs)
{
    for (int i = 0; i < Size; ++i)
    {
        lhs[i] -= rhs[i];
    }

    return lhs;
}

template<typename T, int Size>
vtkVector<T, Size> operator-(vtkVector<T, Size> lhs, const vtkVector<T, Size> & rhs)
{
    lhs -= rhs;

    return lhs;
}

template<typename T, int Size>
vtkVector<T, Size> & operator*=(vtkVector<T, Size> & lhs, const vtkVector<T, Size> & rhs)
{
    for (int i = 0; i < Size; ++i)
    {
        lhs[i] *= rhs[i];
    }

    return lhs;
}

template<typename T, int Size>
vtkVector<T, Size> operator*(vtkVector<T, Size> lhs, const vtkVector<T, Size> & rhs)
{
    lhs *= rhs;

    return lhs;
}

template<typename T, int Size>
vtkVector<T, Size> & operator/=(vtkVector<T, Size> & lhs, const vtkVector<T, Size> & rhs)
{
    for (int i = 0; i < Size; ++i)
    {
        lhs[i] /= rhs[i];
    }

    return lhs;
}

template<typename T, int Size>
vtkVector<T, Size> operator/(vtkVector<T, Size> lhs, const vtkVector<T, Size> & rhs)
{
    lhs /= rhs;

    return lhs;
}


template<typename T, int Size>
vtkVector<T, Size> & operator+=(vtkVector<T, Size> & lhs, const T & scalar)
{
    for (int i = 0; i < Size; ++i)
    {
        lhs[i] += scalar;
    }

    return lhs;
}

template<typename T, int Size>
vtkVector<T, Size> operator+(vtkVector<T, Size> lhs, const T & scalar)
{
    lhs += scalar;

    return lhs;
}

template<typename T, int Size>
vtkVector<T, Size> & operator-=(vtkVector<T, Size> & lhs, const T & scalar)
{
    for (int i = 0; i < Size; ++i)
    {
        lhs[i] -= scalar;
    }

    return lhs;
}

template<typename T, int Size>
vtkVector<T, Size> operator-(vtkVector<T, Size> lhs, const T & scalar)
{
    lhs -= scalar;

    return lhs;
}

template<typename T, int Size>
vtkVector<T, Size> & operator*=(vtkVector<T, Size> & lhs, const T & scalar)
{
    for (int i = 0; i < Size; ++i)
    {
        lhs[i] *= scalar;
    }

    return lhs;
}

template<typename T, int Size>
vtkVector<T, Size> operator*(vtkVector<T, Size> lhs, const T & scalar)
{
    lhs *= scalar;

    return lhs;
}

template<typename T, int Size>
vtkVector<T, Size> & operator/=(vtkVector<T, Size> & lhs, const T & scalar)
{
    for (int i = 0; i < Size; ++i)
    {
        lhs[i] /= scalar;
    }

    return lhs;
}

template<typename T, int Size>
vtkVector<T, Size> operator/(vtkVector<T, Size> lhs, const T & scalar)
{
    lhs /= scalar;

    return lhs;
}


template<typename T, int Size>
vtkVector<T, Size> operator+(const T & scalar, vtkVector<T, Size> rhs)
{
    for (int i = 0; i < Size; ++i)
    {
        rhs[i] += scalar;
    }

    return rhs;
}

template<typename T, int Size>
vtkVector<T, Size> operator-(const T & scalar, vtkVector<T, Size> rhs)
{
    for (int i = 0; i < Size; ++i)
    {
        rhs[i] = scalar - rhs[i];
    }

    return rhs;
}

template<typename T, int Size>
vtkVector<T, Size> operator*(const T & scalar, vtkVector<T, Size> rhs)
{
    for (int i = 0; i < Size; ++i)
    {
        rhs[i] *= scalar;
    }

    return rhs;
}

template<typename T, int Size>
vtkVector<T, Size> operator/(const T & scalar, vtkVector<T, Size> rhs)
{
    for (int i = 0; i < Size; ++i)
    {
        rhs[i] = scalar / rhs[i];
    }

    return rhs;
}

template<typename T, int Size>
vtkVector<T, Size> operator-(vtkVector<T, Size> rhs)
{
    for (int i = 0; i < Size; ++i)
    {
        rhs[i] = -rhs[i];
    }

    return rhs;
}

template<int SizeOut, int SizeIn, typename T>
typename std::enable_if<(SizeOut < SizeIn), vtkVector<T, SizeOut>>::type
convertTo(const vtkVector<T, SizeIn> & other)
{
    return vtkVector<T, SizeOut>(other.GetData());
}

template<int SizeOut, int SizeIn, typename T>
typename std::enable_if<(SizeOut == SizeIn), vtkVector<T, SizeOut>>::type
convertTo(vtkVector<T, SizeIn> other, T /*initValue*/)
{
    return other;
}

template<int SizeOut, int SizeIn, typename T>
typename std::enable_if<(SizeOut > SizeIn), vtkVector<T, SizeOut>>::type
convertTo(const vtkVector<T, SizeIn> & other, const T initValue)
{
    vtkVector<T, SizeOut> result;

    for (int i = 0; i < SizeIn; ++i)
    {
        result[i] = other[i];
    }

    for (int i = SizeIn; i < SizeOut; ++i)
    {
        result[i] = initValue;
    }

    return result;
}

template<typename TOut, typename TIn, int Size>
typename std::enable_if<std::is_same<TOut, TIn>::value, vtkVector<TOut, Size>>::type
convertTo(vtkVector<TIn, Size> other)
{
    return other;
}

template<typename TOut, typename TIn, int Size>
typename std::enable_if<!std::is_same<TOut, TIn>::value, vtkVector<TOut, Size>>::type
convertTo(const vtkVector<TIn, Size> & other)
{
    vtkVector<TOut, Size> result;

    for (int i = 0; i < Size; ++i)
    {
        result[i] = static_cast<TOut>(other[i]);
    }

    return result;
}

template<typename T, int Size>
vtkVector<T, Size> abs(const vtkVector<T, Size> & other)
{
    vtkVector<T, Size> result;

    for (int i = 0; i < Size; ++i)
    {
        result[i] = std::abs(other[i]);
    }

    return result;
}

template<typename T, int Size>
T minComponent(const vtkVector<T, Size> & vector)
{
    return *std::min_element(vector.GetData(), vector.GetData() + vector.GetSize());
}

/** @return the value of the component with the highest value */
template<typename T, int Size>
T maxComponent(const vtkVector<T, Size> & vector)
{
    return *std::max_element(vector.GetData(), vector.GetData() + vector.GetSize());
}

template<typename T, int Size>
vtkVector<T, Size> min(const vtkVector<T, Size> & lhs, const vtkVector<T, Size> & rhs)
{
    vtkVector<T, Size> result;

    for (int i = 0; i < Size; ++i)
    {
        result[i] = std::min(lhs[i], rhs[i]);
    }

    return result;
}

template<typename T, int Size>
vtkVector<T, Size> max(const vtkVector<T, Size> & lhs, const vtkVector<T, Size> & rhs)
{
    vtkVector<T, Size> result;

    for (int i = 0; i < Size; ++i)
    {
        result[i] = std::max(lhs[i], rhs[i]);
    }

    return result;
}

template<typename T, int Size>
typename std::enable_if<std::numeric_limits<T>::has_quiet_NaN, void>::type
uninitializeVector(vtkVector<T, Size> & vector)
{
    const auto n = std::numeric_limits<T>::quiet_NaN();
    for (int i = 0; i < Size; ++i)
    {
        vector[i] = n;
    }
}

template<typename T, int Size>
typename std::enable_if<std::numeric_limits<T>::has_quiet_NaN, vtkVector<T, Size>>::type
uninitializedVector()
{
    vtkVector<T, Size> vector;
    uninitializeVector(vector);
    return vector;
}

template<typename T, int Size>
typename std::enable_if<std::numeric_limits<T>::has_quiet_NaN, bool>::type
isVectorInitialized(const vtkVector<T, Size> & vector)
{
    for (int i = 0; i < Size; ++i)
    {
        if (std::isnan(vector[i]))
        {
            return false;
        }
    }

    return true;
}
