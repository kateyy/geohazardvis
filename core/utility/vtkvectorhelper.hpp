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
