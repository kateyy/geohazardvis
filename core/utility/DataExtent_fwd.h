#pragma once


template<typename T, size_t Dimensions>
class DataExtent;


using ImageExtent = DataExtent<int, 3u>;
using DataBounds = DataExtent<double, 3u>;
template<typename T = double> using ValueRange = DataExtent<T, 1u>;
