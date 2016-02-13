#pragma once

#include <array>

#include <vtkVector.h>


template<typename T, size_t Dimensions = 3u>
class DataExtent
{
public:
    static const auto ValueCount = Dimensions * 2u;
    using array_t = std::array<T, ValueCount>;

    DataExtent();
    explicit DataExtent(array_t extent);
    explicit DataExtent(const T extent[ValueCount]);

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

    template<typename newT, size_t newDimensions = Dimensions>
    DataExtent<newT, newDimensions> convertTo() const;

    template<size_t newDimensions>
    DataExtent<T, newDimensions> convertTo() const;

    const T * data() const;
    // unsafe workaround for non-const VTK interfaces (e.g., vtkImageData)
    T * data();


    void add(const DataExtent & other);
    void add(const T other[ValueCount]);

    static DataExtent intersect(DataExtent lhs, const DataExtent & rhs);

    DataExtent & intersect(const DataExtent & other);

private:
    array_t m_extent;
};

using ImageExtent = DataExtent<int, 3>;
using DataBounds = DataExtent<double, 3>;


#include <core/utility/DataExtent.hpp>
