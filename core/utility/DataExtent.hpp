#pragma once

#include <core/utility/DataExtent.h>

#include <algorithm>
#include <limits>
#include <type_traits>


template<typename T, size_t Dimensions>
DataExtent<T, Dimensions>::DataExtent()
{
    static_assert(size_t(int(Dimensions)) == Dimensions, "Data Extent type is not compatible with VTK's vector type");

    for (size_t i = 0; i < Dimensions; ++i)
    {
        m_extent[2 * i] = std::numeric_limits<T>::max();
        m_extent[2 * i + 1] = std::numeric_limits<T>::lowest();
    }
}

template<typename T, size_t Dimensions>
DataExtent<T, Dimensions>::DataExtent(array_t extent)
    : m_extent(extent)
{
}

template<typename T, size_t Dimensions>
DataExtent<T, Dimensions>::DataExtent(const T extent[ValueCount])
{
    for (size_t i = 0; i < ValueCount; ++i)
    {
        m_extent[i] = extent[i];
    }
}

template<typename T, size_t Dimensions>
bool DataExtent<T, Dimensions>::isEmpty() const
{
    for (size_t i = 0; i < Dimensions; ++i)
    {
        if (m_extent[2 * i] > m_extent[2 * i + 1])
        {
            return true;
        }
    }

    return false;
}

template<typename T, size_t Dimensions>
DataExtent<T, Dimensions>::operator array_t() const
{
    return m_extent;
}

template<typename T, size_t Dimensions>
T DataExtent<T, Dimensions>::operator[] (size_t index) const
{
    return m_extent[index];
}

template<typename T, size_t Dimensions>
T & DataExtent<T, Dimensions>::operator[] (size_t index)
{
    return m_extent[index];
}

template<typename T, size_t Dimensions>
bool DataExtent<T, Dimensions>::operator==(const T other[ValueCount]) const
{
    for (size_t i = 0; i < ValueCount; ++i)
    {
        if (m_extent[i] != other[i])
            return false;
    }
    return true;
}

template<typename T, size_t Dimensions>
bool DataExtent<T, Dimensions>::operator==(const DataExtent & other) const
{
    return operator==(other.m_extent.data());
}

template<typename T, size_t Dimensions>
const T * DataExtent<T, Dimensions>::data() const
{
    return m_extent.data();
}

template<typename T, size_t Dimensions>
T * DataExtent<T, Dimensions>::data()
{
    return m_extent.data();
}

template<typename T, size_t Dimensions>
template<typename newT, size_t newDimensions>
DataExtent<newT, newDimensions> DataExtent<T, Dimensions>::convertTo() const
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
vtkVector<T, Dimensions> DataExtent<T, Dimensions>::center() const
{
    vtkVector<T, Dimensions> result;

    for (size_t i = 0; i < Dimensions; ++i)
    {
        auto int_i = static_cast<int>(i);
        result[int_i] = static_cast<T>(0.5 * m_extent[2 * i] + 0.5 * m_extent[2 * i + 1]);
    }

    return result;
}

template<typename T, size_t Dimensions>
vtkVector<T, Dimensions> DataExtent<T, Dimensions>::size() const
{
    vtkVector<T, Dimensions> result;

    for (size_t i = 0; i < Dimensions; ++i)
    {
        auto int_i = static_cast<int>(i);
        result[int_i] = m_extent[2 * i + 1] - m_extent[2 * i];
    }

    return result;
}

template<typename T, size_t Dimensions>
void DataExtent<T, Dimensions>::add(const T other[ValueCount])
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
void DataExtent<T, Dimensions>::add(const DataExtent & other)
{
    Add(other.m_extent);
}

template<typename T, size_t Dimensions>
auto DataExtent<T, Dimensions>::intersect(DataExtent lhs, const DataExtent & rhs) -> DataExtent
{
    lhs.intersect(rhs);

    return lhs;
}

template<typename T, size_t Dimensions>
auto DataExtent<T, Dimensions>::intersect(const DataExtent & other) -> DataExtent &
{
    if (isEmpty() || other.isEmpty())
    {
        return *this;
    }

    for (size_t i = 0u; i < Dimensions; ++i)
    {
        const auto minIdx = 2u * i;
        const auto maxIdx = 2u * i + 1u;

        if (m_extent[minIdx] < other.m_extent[minIdx])
            m_extent[minIdx] = other.m_extent[minIdx];
        if (m_extent[maxIdx] > other.m_extent[maxIdx])
            m_extent[maxIdx] = other.m_extent[maxIdx];

        if (m_extent[minIdx] > m_extent[maxIdx])
        {
            *this = DataExtent();
            break;
        }
    }

    return *this;
}
