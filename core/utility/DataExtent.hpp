#pragma once

#include <core/utility/DataExtent.h>

#include <algorithm>
#include <limits>
#include <type_traits>


template<typename T, size_t Dimensions>
DataExtent<T, Dimensions>::DataExtent()
{
    static_assert(size_t(int(Dimensions)) == Dimensions, "Data Extent type is not compatible with VTK's vector type");

    for (size_t i = 0u; i < Dimensions; ++i)
    {
        m_extent[2u * i] = std::numeric_limits<T>::max();
        m_extent[2u * i + 1u] = std::numeric_limits<T>::lowest();
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
    for (size_t i = 0u; i < ValueCount; ++i)
    {
        m_extent[i] = extent[i];
    }
}

/** Incompatible with Visual Studio 2013 Compiler :( */
//template<typename T, size_t Dimensions>
//template<size_t ValueCount1, class...T2, typename std::enable_if<sizeof...(T2) == ValueCount1, int>::type>
//DataExtent<T, Dimensions>::DataExtent(T2... args)
//    : m_extent{ args... }
//{
//}

template<typename T, size_t Dimensions>
bool DataExtent<T, Dimensions>::isEmpty() const
{
    for (size_t i = 0u; i < Dimensions; ++i)
    {
        if (m_extent[2u * i] > m_extent[2u * i + 1u])
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
    for (size_t i = 0u; i < ValueCount; ++i)
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
bool DataExtent<T, Dimensions>::operator!=(const T other[ValueCount]) const
{
    return !(*this == other);
}

template<typename T, size_t Dimensions>
bool DataExtent<T, Dimensions>::operator!=(const DataExtent & other) const
{
    return !(*this == other);
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

    for (size_t i = 0u; i < initValues; ++i)
    {
        result[i] = m_extent[i];
    }

    return result;
}

template<typename T, size_t Dimensions>
template<size_t newDimensions>
DataExtent<T, newDimensions> DataExtent<T, Dimensions>::convertTo() const
{
    return convertTo<T, newDimensions>();
}

template<typename T, size_t Dimensions>
template<size_t Dimensions1>
typename std::enable_if<(Dimensions1 == 1u), T>::type
DataExtent<T, Dimensions>::center() const
{
    return static_cast<T>(0.5 * m_extent[0u] + 0.5 * m_extent[1u]);
}

template<typename T, size_t Dimensions>
template<size_t Dimensions1>
typename std::enable_if<(Dimensions1 > 1u), vtkVector<T, Dimensions>>::type
DataExtent<T, Dimensions>::center() const
{
    vtkVector<T, Dimensions> result;

    for (size_t i = 0u; i < Dimensions; ++i)
    {
        const auto int_i = static_cast<int>(i);
        result[int_i] = static_cast<T>(0.5 * m_extent[2u * i] + 0.5 * m_extent[2u * i + 1u]);
    }

    return result;
}

template<typename T, size_t Dimensions>
template<size_t Dimensions1>
typename std::enable_if<(Dimensions1 == 1u), T>::type
DataExtent<T, Dimensions>::componentSize() const
{
    return m_extent[1u] - m_extent[0u];
}

template<typename T, size_t Dimensions>
template<size_t Dimensions1>
typename std::enable_if<(Dimensions1 > 1u), vtkVector<T, Dimensions>>::type
DataExtent<T, Dimensions>::componentSize() const
{
    vtkVector<T, Dimensions> result;

    for (size_t i = 0u; i < Dimensions; ++i)
    {
        const auto int_i = static_cast<int>(i);
        result[int_i] = m_extent[2u * i + 1u] - m_extent[2u * i];
    }

    return result;
}

template<typename T, size_t Dimensions>
template<size_t Dimensions1>
typename std::enable_if<(Dimensions1 == 1u), T>::type
DataExtent<T, Dimensions>::min() const
{
    return m_extent[0u];
}

template<typename T, size_t Dimensions>
template<size_t Dimensions1>
typename std::enable_if<(Dimensions1 > 1u), vtkVector<T, Dimensions>>::type
DataExtent<T, Dimensions>::min() const
{
    vtkVector<T, Dimensions> m;

    for (int i = 0u; i < int(Dimensions); ++i)
    {
        m[i] = m_extent[2u * i];
    }

    return m;
}

template<typename T, size_t Dimensions>
template<size_t Dimensions1>
typename std::enable_if<(Dimensions1 == 1u), T>::type
DataExtent<T, Dimensions>::max() const
{
    return m_extent[1u];
}

template<typename T, size_t Dimensions>
template<size_t Dimensions1>
typename std::enable_if<(Dimensions1 > 1u), vtkVector<T, Dimensions>>::type
DataExtent<T, Dimensions>::max() const
{
    vtkVector<T, Dimensions> m;

    for (int i = 0u; i < int(Dimensions); ++i)
    {
        m[i] = m_extent[2u * i + 1u];
    }

    return m;
}

template<typename T, size_t Dimensions>
ValueRange<T> DataExtent<T, Dimensions>::extractDimension(size_t dimension) const
{
    return DataExtent<T, 1u>({ m_extent[2u * dimension], m_extent[2u * dimension + 1u] });
}

template<typename T, size_t Dimensions>
auto DataExtent<T, Dimensions>::setDimension(size_t dimension, const ValueRange<T> & range) -> DataExtent &
{
    return setDimension(range[0u], range[1u]);
}

template<typename T, size_t Dimensions>
auto DataExtent<T, Dimensions>::setDimension(size_t dimension, T min, T max) -> DataExtent &
{
    m_extent[2u * dimension] = min;
    m_extent[2u * dimension + 1u] = max;

    return *this;
}

template<typename T, size_t Dimensions>
auto DataExtent<T, Dimensions>::add(const DataExtent & other) -> DataExtent &
{
    if (other.isEmpty())
    {
        return *this;
    }

    if (isEmpty())
    {
        *this = other;
        return *this;
    }

    for (size_t i = 0u; i < Dimensions; ++i)
    {
        if (m_extent[2u * i] > other[2u * i])
            m_extent[2u * i] = other[2u * i];

        if (m_extent[2u * i + 1u] < other[2u * i + 1u])
            m_extent[2u * i + 1u] = other[2u * i + 1u];
    }

    return *this;
}

template<typename T, size_t Dimensions>
auto DataExtent<T, Dimensions>::sum(DataExtent other) const -> DataExtent
{
    other.add(*this);

    return other;
}

template<typename T, size_t Dimensions>
auto DataExtent<T, Dimensions>::intersection(DataExtent other) const -> DataExtent
{
    other.intersect(*this);

    return other;
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

template<typename T, size_t Dimensions>
template<size_t Dimensions1>
typename std::enable_if<(Dimensions1 == 1u), bool>::type
DataExtent<T, Dimensions>::contains(T value) const
{
    return m_extent[0u] <= value && value <= m_extent[1];
}

template<typename T, size_t Dimensions>
template<size_t Dimensions1>
typename std::enable_if<(Dimensions1 > 1u), bool>::type
DataExtent<T, Dimensions>::contains(const vtkVector<T, Dimensions> & point) const
{
    for (size_t i = 0u; i < Dimensions; ++i)
    {
        if (m_extent[2u * i] > point[static_cast<int>(i)]
            || m_extent[2u * i + 1u] < point[static_cast<int>(i)])
        {
            return false;
        }
    }

    return true;
}

template<typename T, size_t Dimensions>
bool DataExtent<T, Dimensions>::contains(const DataExtent & other) const
{
    return
        contains(other.min())
        && contains(other.max());
}

template<typename T, size_t Dimensions>
template<size_t Dimensions1>
typename std::enable_if<(Dimensions1 == 1u), T>::type
DataExtent<T, Dimensions>::clampValue(T value) const
{
    return std::max(m_extent[0u], std::min(m_extent[1u], value));
}

template<typename T, size_t Dimensions>
template<size_t Dimensions1>
typename std::enable_if<(Dimensions1 > 1u), vtkVector<T, Dimensions>>::type
DataExtent<T, Dimensions>::clampPoint(vtkVector<T, Dimensions> point) const
{
    for (size_t i = 0u; i < Dimensions; ++i)
    {
        const auto int_i = static_cast<int>(i);
        point[int_i] = std::max(m_extent[2u * i],
            std::min(m_extent[2u * i + 1u], point[int_i]));
    }

    return point;
}

template<typename T, size_t Dimensions>
template<typename T1>
typename std::enable_if<std::is_integral<T1>::value, size_t>::type
DataExtent<T, Dimensions>::numberOfPoints() const
{
    if (isEmpty())
    {
        return 0u;
    }

    size_t numPoints = 1u;

    for (size_t i = 0u; i < Dimensions; ++i)
    {
        numPoints *= m_extent[2u * i + 1u] - m_extent[2u * i] + 1u;
    }

    return numPoints;
}

template<typename T, size_t Dimensions>
template<typename T1>
typename std::enable_if<std::is_integral<T1>::value, size_t>::type
DataExtent<T, Dimensions>::numberOfCells() const
{
    if (isEmpty())
    {
        return 0u;
    }

    size_t numCells = 1u;

    for (size_t i = 0u; i < Dimensions; ++i)
    {
        numCells *= m_extent[2u * i + 1u] - m_extent[2u * i];
    }

    return numCells;
}
