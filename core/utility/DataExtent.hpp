
#include <core/utility/DataExtent.h>

#include <algorithm>
#include <cassert>
#include <cmath>
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

template<typename T, size_t Dimensions>
template<typename newT, size_t newDimensions>
DataExtent<newT, newDimensions> DataExtent<T, Dimensions>::ConvertTo() const
{
    DataExtent<newT, newDimensions> result;

    size_t initValues = 2u * std::min(Dimensions, newDimensions);

    for (size_t i = 0; i < initValues; ++i)
    {
        result[i] = m_extent[i];
    }

    return result;
}

template<typename T, size_t Dimensions>
vtkVector<T, Dimensions> DataExtent<T, Dimensions>::Center() const
{
    vtkVector<T, Dimensions> result;

    for (size_t i = 0; i < Dimensions; ++i)
    {
        result[i] = static_cast<T>(0.5 * m_extent[2 * i] + 0.5 * m_extent[2 * i + 1]);
    }

    return result;
}

template<typename T, size_t Dimensions>
vtkVector<T, Dimensions> DataExtent<T, Dimensions>::Size() const
{
    vtkVector<T, Dimensions> result;

    for (size_t i = 0; i < Dimensions; ++i)
    {
        result[i] = m_extent[2 * i + 1] - m_extent[2 * i];
    }

    return result;
}
