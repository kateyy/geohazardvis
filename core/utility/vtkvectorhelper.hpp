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


template<int SizeOut, int SizeIn, typename T>
typename std::enable_if<(SizeOut < SizeIn), vtkVector<T, SizeOut>>::type
convert(const vtkVector<T, SizeIn> & other)
{
    return vtkVector<T, SizeOut>(other.GetData());
}

template<int SizeOut, int SizeIn, typename T>
typename std::enable_if<(SizeOut > SizeIn), vtkVector<T, SizeOut>>::type
convert(const vtkVector<T, SizeIn> & other, const T initValue)
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
