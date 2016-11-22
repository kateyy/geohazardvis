#pragma once

#include <iomanip>

#include <core/utility/DataExtent_print.h>


/** Workaround for gtest not selecting the expected PrintTo or operator<<
 * (Cannot work for template classes)
 * https://stackoverflow.com/questions/25146997/teach-google-test-how-to-print-eigen-matrix
 */
class tDataBounds : public DataBounds
{
public:
    using DataBounds::DataBounds;
};

class tImageExtent : public ImageExtent
{
public:
    using ImageExtent::ImageExtent;
};


inline void PrintTo(const tDataBounds & bounds, std::ostream * os)
{
    *os  << std::setprecision(17) << bounds;
}

inline void PrintTo(const tImageExtent & extent, std::ostream * os)
{
    *os << extent;
}
