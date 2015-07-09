#pragma once

#include <memory>
#include <vector>


template<typename InIt, typename Ty>
InIt findUnique(InIt First, InIt Last, const Ty * Val);

template<typename T>
typename std::vector<std::unique_ptr<T>>::iterator findUnique(std::vector<std::unique_ptr<T>> & vector, const T * Val);


#include <core/utility/memory.hpp>
