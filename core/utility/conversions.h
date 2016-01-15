#pragma once

#include <array>

#include <QString>

#include <vtkVector.h>

#include <core/io/types.h>


template<typename FPType, size_t Size>
QString arrayToString(const std::array<FPType, Size> & a);

template<typename FPType, size_t Size>
void stringToArray(const QString & s, std::array<FPType, Size> & resultArray);

template<typename FPType, size_t Size>
std::array<FPType, Size> stringToArray(const QString & s);

template<typename FPType>
void stringToVector3(const QString & s, vtkVector3<FPType> & resultVector);

template<typename FPType>
vtkVector3<FPType> stringToVector3(const QString & s);

template<typename FPType>
QString vector3ToString(const vtkVector3<FPType> & v);

template<typename T, int Size>
QString vectorToString(const vtkVector<T, Size> & vector,
    const QString & separator = " ", const QString & prefix = "", const QString & suffix = "");


#include "conversions.hpp"
