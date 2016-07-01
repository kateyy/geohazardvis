#pragma once

#include <array>

#include <vtkVector.h>


template<typename T, size_t Dimensions = 3u>
class DataExtent
{
public:
    using value_type = T;
    static const auto ValueCount = Dimensions * 2u;
    using array_t = std::array<T, ValueCount>;

    DataExtent();
    explicit DataExtent(array_t extent);
    explicit DataExtent(const T extent[ValueCount]);

    /** Incompatible with Visual Studio 2013 Compiler :( */
    //template<size_t ValueCount1 = ValueCount, class...T2, typename std::enable_if<sizeof...(T2) == ValueCount1, int>::type = 0>
    //explicit DataExtent(T2... args);

    bool isEmpty() const;

    operator array_t () const;

    T operator[] (size_t index) const;
    T & operator[] (size_t index);

    bool operator==(const T other[ValueCount]) const;
    bool operator==(const DataExtent & other) const;

    bool operator!=(const T other[ValueCount]) const;
    bool operator!=(const DataExtent & other) const;

    vtkVector<T, Dimensions> center() const;
    vtkVector<T, Dimensions> size() const;

    vtkVector<T, Dimensions> minRange() const;
    vtkVector<T, Dimensions> maxRange() const;

    vtkVector2<T> extractDimension(size_t dimension) const;
    DataExtent & setDimension(size_t dimension, const vtkVector2<T> & range);

    template<typename newT, size_t newDimensions = Dimensions>
    DataExtent<newT, newDimensions> convertTo() const;

    template<size_t newDimensions>
    DataExtent<T, newDimensions> convertTo() const;

    const T * data() const;
    // unsafe workaround for non-const VTK interfaces (e.g., vtkImageData)
    T * data();


    DataExtent & add(const DataExtent & other);
    DataExtent sum(DataExtent other) const;

    DataExtent intersection(DataExtent other) const;
    DataExtent & intersect(const DataExtent & other);

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 == 1u), bool>::type
        contains(T value) const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 > 1u), bool>::type
        contains(const vtkVector<T, Dimensions> & point) const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 == 1u), T>::type
        clampValue(T value) const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 > 1u), vtkVector<T, Dimensions>>::type
        clampPoint(vtkVector<T, Dimensions> point) const;

    template<typename T1 = T>
    typename std::enable_if<std::is_integral<T1>::value, size_t>::type
        numberOfPoints() const;

    template<typename T1 = T>
    typename std::enable_if<std::is_integral<T1>::value, size_t>::type
        numberOfCells() const;

private:
    array_t m_extent;
};

using ImageExtent = DataExtent<int, 3>;
using DataBounds = DataExtent<double, 3>;


#include <core/utility/DataExtent.hpp>
