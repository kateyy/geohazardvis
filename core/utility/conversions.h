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
