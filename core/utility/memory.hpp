#pragma once

#include <core/utility/memory.h>


template<typename InIt, typename Ty>
InIt findUnique(InIt First, InIt Last, const Ty * Val)
{
    auto it = First;
    for (; it != Last; ++it)
    {
        if (it->get() == Val)
            break;
    }
    return it;
}

template<typename T>
typename std::vector<std::unique_ptr<T>>::iterator findUnique(std::vector<std::unique_ptr<T>> & vector, const T * Val)
{
    return findUnique(vector.begin(), vector.end(), Val);
}
