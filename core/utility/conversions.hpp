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

#include <core/utility/conversions.h>


template<typename ValueType, size_t Size>
QString arrayToString(const std::array<ValueType, Size> & a,
    const QString & separator, const QString & prefix, const QString & suffix)
{
    QString s;
    for (size_t i = 0u; i < Size; ++i)
    {
        s += prefix + QString::number(a[i]) + suffix + separator;
    }

    s.remove(s.length() - separator.length(), separator.length());

    return s;
}

template<typename ValueType, size_t Size>
void stringToArray(const QString & s, std::array<ValueType, Size> & resultArray)
{
    const auto parts = s.split(" ");
    const auto numParts = static_cast<size_t>(parts.size());
    if (numParts < Size)
    {
        resultArray = {};
        return;
    }

    for (size_t i = 0u; i < numParts; ++i)
    {
        resultArray[i] = static_cast<ValueType>(parts[static_cast<int>(i)].toDouble());
    }
}

template<typename ValueType, size_t Size>
std::array<ValueType, Size> stringToArray(const QString & s)
{
    std::array<ValueType, Size> result = {};
    stringToArray(s, result);
    return result;
}

template<typename ValueType, int Size>
vtkVector<ValueType, Size> stringToVector(const QString & s)
{
    vtkVector<ValueType, Size> result;

    stringToVector(s, result);

    return result;
}

template<typename ValueType, int Size>
void stringToVector(const QString & s, vtkVector<ValueType, Size> & resultVector)
{
    const auto array = stringToArray<ValueType, static_cast<size_t>(Size)>(s);
    resultVector = reinterpret_cast<const vtkVector<ValueType, Size> &>(array);
}

template<typename ValueType>
void stringToVector2(const QString & s, vtkVector2<ValueType> & resultVector)
{
    stringToVector(s, resultVector);
}

template<typename ValueType>
vtkVector2<ValueType> stringToVector2(const QString & s)
{
    vtkVector2<ValueType> result;

    stringToVector2(s, result);

    return result;
}

template<typename ValueType>
void stringToVector3(const QString & s, vtkVector3<ValueType> & resultVector)
{
    stringToVector(s, resultVector);
}

template<typename ValueType>
vtkVector3<ValueType> stringToVector3(const QString & s)
{
    vtkVector3<ValueType> result;

    stringToVector3(s, result);

    return result;
}

template<typename ValueType>
QString vector3ToString(const vtkVector3<ValueType> & v)
{
    return vectorToString(v);
}

template<typename ValueType, int Size>
QString vectorToString(const vtkVector<ValueType, Size> & vector,
    const QString & separator, const QString & prefix, const QString & suffix)
{
    return arrayToString(
        reinterpret_cast<const std::array<ValueType, static_cast<size_t>(Size)> &>(vector),
        separator, prefix, suffix);
}
