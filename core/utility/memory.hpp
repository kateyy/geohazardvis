#pragma once

#include <core/utility/memory.h>


template<typename InIt, typename Ty>
InIt findUnique(InIt First, InIt Last, const Ty * value)
{
    auto it = First;
    for (; it != Last; ++it)
    {
        if (it->get() == value)
            break;
    }
    return it;
}

template<typename T>
typename std::vector<std::unique_ptr<T>>::iterator findUnique(std::vector<std::unique_ptr<T>> & vector, const T * value)
{
    return findUnique(vector.begin(), vector.end(), value);
}

template<typename T>
typename std::vector<std::unique_ptr<T>>::const_iterator findUnique(const std::vector<std::unique_ptr<T>> & vector, const T * value)
{
    return findUnique(vector.begin(), vector.end(), value);
}

template<typename T>
bool containsUnique(const std::vector<std::unique_ptr<T>> & vector, const T * value)
{
    return findUnique(vector, value) != vector.end();
}
