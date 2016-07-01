#pragma once

#include <algorithm>
#include <cmath>
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
convert(const vtkVector<T, SizeIn> & other);

template<int SizeOut, int SizeIn, typename T>
typename std::enable_if<(SizeOut > SizeIn), vtkVector<T, SizeOut>>::type
convert(const vtkVector<T, SizeIn> & other, const T initValue = T());

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

#include "vtkvectorhelper.hpp"
