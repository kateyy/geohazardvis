/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <array>

#include <vtkVector.h>

#include <core/utility/DataExtent_fwd.h>


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
    explicit DataExtent(std::array<ValueRange<T>, Dimensions> componentValueRanges);

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

    bool isApprox(const T other[ValueCount], T absError) const;
    bool isApprox(const DataExtent & other, T absError) const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 == 1u), T>::type
        center() const;
    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 > 1u), vtkVector<T, Dimensions>>::type
        center() const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 == 1u), T>::type
        componentSize() const;
    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 > 1u), vtkVector<T, Dimensions>>::type
        componentSize() const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 == 1u), T>::type
        min() const;
    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 > 1u), vtkVector<T, Dimensions>>::type
        min() const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 == 1u), T>::type
        max() const;
    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 > 1u), vtkVector<T, Dimensions>>::type
        max() const;

    ValueRange<T> extractDimension(size_t dimension) const;
    ValueRange<T> & dimension(size_t dimension);
    DataExtent & setDimension(size_t dimension, const ValueRange<T> & range);
    DataExtent & setDimension(size_t dimension, T min, T max);

    template<typename newT, size_t newDimensions = Dimensions>
    DataExtent<newT, newDimensions> convertTo() const;

    template<size_t newDimensions>
    DataExtent<T, newDimensions> convertTo() const;

    const T * data() const;
    // unsafe workaround for non-const VTK interfaces (e.g., vtkImageData)
    T * data();


    DataExtent & add(const DataExtent & other);
    DataExtent sum(DataExtent other) const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 == 1u), DataExtent &>::type
        add(T value);

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 > 1u), DataExtent &>::type
        add(const vtkVector<T, Dimensions> & point);

    /** Returns a copy of this intersected by other. See intersect() */
    DataExtent intersection(DataExtent other) const;
    /** Minima per dimension are set to the highest value of this and other. The inverse is done
    * for the maxima per dimension.
    * An intersection involving an empty extent is always empty. No intersection is computed in
    * this case, instead the extent is reinitialized. */
    DataExtent & intersect(const DataExtent & other);

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 == 1u), DataExtent &>::type
        shift(T shiftValue);

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 > 1u), DataExtent &>::type
        shift(const vtkVector<T, Dimensions> & shiftVector);

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 == 1u), DataExtent>::type
        shifted(T shiftValue) const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 > 1u), DataExtent>::type
        shifted(const vtkVector<T, Dimensions> & shiftVector) const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 == 1u), DataExtent &>::type
        scale(T scaleFactor);

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 > 1u), DataExtent &>::type
        scale(const vtkVector<T, Dimensions> & scaleVector);

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 == 1u), DataExtent>::type
        scaled(T scaleFactor) const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 > 1u), DataExtent>::type
        scaled(const vtkVector<T, Dimensions> & scaleVector) const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 == 1u), bool>::type
        contains(T value) const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 > 1u), bool>::type
        contains(const vtkVector<T, Dimensions> & point) const;

    bool contains(const DataExtent & other) const;

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

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 > 1u), vtkVector<double, Dimensions>>::type
        relativeOriginPosition() const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 == 1u), double>::type
        relativeOriginPosition() const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 > 1u), vtkVector<double, Dimensions>>::type
        relativePositionOf(const vtkVector<T, Dimensions> & point) const;

    template<size_t Dimensions1 = Dimensions>
    typename std::enable_if<(Dimensions1 == 1u), double>::type
        relativePositionOf(T point) const;

private:
    array_t m_extent;
};


#include <core/utility/DataExtent.hpp>
