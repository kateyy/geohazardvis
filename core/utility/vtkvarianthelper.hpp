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

#include "vtkvarianthelper.h"


template<typename ValueType, typename>
ValueType vtkVariantToValue(const vtkVariant & variant, bool * isValid)
{
    return vtkVariantCast<ValueType>(variant, isValid);
}

template<typename ValueType>
std::enable_if_t<std::is_enum<ValueType>::value, ValueType>
vtkVariantToValue(const vtkVariant & variant, bool * isValid)
{
    using underlying_t = std::underlying_type_t<ValueType>;
    return static_cast<ValueType>(vtkVariantToValue<underlying_t>(variant, isValid));
}

template<> inline int vtkTypeForValueType<int32_t>()
{
    return VTK_TYPE_INT32;
}

template<> inline int vtkTypeForValueType<int64_t>()
{
    return VTK_TYPE_INT64;
}

template<> inline int vtkTypeForValueType<uint32_t>()
{
    return VTK_TYPE_UINT32;
}

template<> inline int vtkTypeForValueType<uint64_t>()
{
    return VTK_TYPE_UINT64;
}

template<> inline int vtkTypeForValueType<double>()
{
    return VTK_DOUBLE;
}

template<> inline int vtkTypeForValueType<char>()
{
    return VTK_CHAR;
}

template<typename T, typename, typename Underlying>
inline int vtkTypeForValueType()
{
    return vtkTypeForValueType<Underlying>();
};
