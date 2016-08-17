#pragma once

#include <type_traits>


template<typename T>
using Unqualified = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
