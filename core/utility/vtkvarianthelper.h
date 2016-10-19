#pragma once

#include <type_traits>

#include <vtkVariantCast.h>


template<typename ValueType, typename = std::enable_if_t<std::is_arithmetic<ValueType>::value>>
ValueType vtkVariantToValue(const vtkVariant & variant, bool * isValid = nullptr);

template<typename ValueType>
std::enable_if_t<std::is_enum<ValueType>::value, ValueType>
vtkVariantToValue(const vtkVariant & variant, bool * isValid = nullptr);


template<typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
int vtkTypeForValueType();

template<typename T, typename = std::enable_if_t<std::is_enum<T>::value>, typename Underlying = std::underlying_type_t<T>>
int vtkTypeForValueType();


#include "vtkvarianthelper.hpp"
