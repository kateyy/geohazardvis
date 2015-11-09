#pragma once

#include <vtkVector.h>


template<typename T, size_t Dimensions = 3>
class DataExtent
{
public:
    DataExtent();
    explicit DataExtent(T extent[Dimensions * 2]);

    void Add(const T other[Dimensions * 2]);

    void Add(const DataExtent & other);

    T operator[] (size_t index) const;

    T & operator[] (size_t index);

    bool operator==(const T other[Dimensions * 2]) const;

    bool operator==(const DataExtent & other) const;

    vtkVector<T, Dimensions> Center() const;
    vtkVector<T, Dimensions> Size() const;

    template<typename newT, size_t newDimensions>
    DataExtent<newT, newDimensions> ConvertTo() const;

    const T * Data() const;

    // unsafe workaround for non-const VTK interfaces (e.g., vtkImageData)
    T * Data();

private:
    T m_extent[Dimensions * 2];
};

using ImageExtent = DataExtent<int, 3>;


#include <core/utility/DataExtent.hpp>
