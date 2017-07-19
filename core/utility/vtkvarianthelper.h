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

#include <type_traits>

#include <vtkObject.h>
#include <vtkVariant.h>
#include <vtkVariantCast.h>

#include <core/core_api.h>


class QVariant;


template<typename ValueType, typename = std::enable_if_t<std::is_arithmetic<ValueType>::value>>
ValueType vtkVariantToValue(const vtkVariant & variant, bool * isValid = nullptr);

template<typename ValueType>
std::enable_if_t<std::is_enum<ValueType>::value, ValueType>
vtkVariantToValue(const vtkVariant & variant, bool * isValid = nullptr);


template<typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
int vtkTypeForValueType();

template<typename T, typename = std::enable_if_t<std::is_enum<T>::value>, typename Underlying = std::underlying_type_t<T>>
int vtkTypeForValueType();


CORE_API QVariant vtkVariantToQVariant(const vtkVariant & variant);


#include "vtkvarianthelper.hpp"
