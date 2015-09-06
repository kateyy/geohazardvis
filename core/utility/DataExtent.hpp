
#include <core/utility/DataExtent.h>

#include <algorithm>
#include <cassert>
#include <limits>


template<typename T, size_t Dimensions>
DataExtent<T, Dimensions>::DataExtent()
{
    for (size_t i = 0; i < Dimensions; ++i)
    {
        m_extent[2 * i] = std::numeric_limits<T>::max();
        m_extent[2 * i + 1] = std::numeric_limits<T>::lowest();
    }
}

template<typename T, size_t Dimensions>
DataExtent<T, Dimensions>::DataExtent(T extent[Dimensions * 2])
{
    std::copy(extent, extent + Dimensions * 2, m_extent);
}

template<typename T, size_t Dimensions>
void DataExtent<T, Dimensions>::Add(const T other[Dimensions * 2])
{
    for (size_t i = 0; i < Dimensions; ++i)
    {
        if (m_extent[2 * i] < other[2 * i])
            m_extent[2 * i] = other[2 * i];

        if (m_extent[2 * i + 1] > other[2 * i + 1])
            m_extent[2 * i + 1] = other[2 * i + 1];
    }
}

template<typename T, size_t Dimensions>
void DataExtent<T, Dimensions>::Add(const DataExtent & other)
{
    Add(other.m_extent);
}

template<typename T, size_t Dimensions>
T DataExtent<T, Dimensions>::operator[] (size_t index) const
{
    assert(index < Dimensions * 2);
    return m_extent[index];
}

template<typename T, size_t Dimensions>
T & DataExtent<T, Dimensions>::operator[] (size_t index)
{
    assert(index < Dimensions * 2);
    return m_extent[index];
}

template<typename T, size_t Dimensions>
bool DataExtent<T, Dimensions>::operator==(const T other[Dimensions * 2]) const
{
    for (size_t i = 0; i < Dimensions * 2; ++i)
    {
        if (m_extent[i] != other[i])
            return false;
    }
    return true;
}

template<typename T, size_t Dimensions>
bool DataExtent<T, Dimensions>::operator==(const DataExtent & other) const
{
    return operator==(other.m_extent);
}

template<typename T, size_t Dimensions>
const T * DataExtent<T, Dimensions>::Data() const
{
    return m_extent;
}

template<typename T, size_t Dimensions>
T * DataExtent<T, Dimensions>::Data()
{
    return m_extent;
}
