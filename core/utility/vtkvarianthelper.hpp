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
