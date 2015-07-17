#pragma once

#include <memory>
#include <vector>


template<typename InIt, typename Ty>
InIt findUnique(InIt First, InIt Last, const Ty * value);

template<typename T>
typename std::vector<std::unique_ptr<T>>::iterator findUnique(std::vector<std::unique_ptr<T>> & vector, const T * value);

template<typename T>
bool containsUnique(std::vector<std::unique_ptr<T>> & vector, const T * value);


#include <core/utility/memory.hpp>
