#pragma once

#include "DataExtent_print.h"


template<typename T, size_t Dimensions>
std::ostream & operator<<(std::ostream & os, const DataExtent<T, Dimensions> & dataExtent)
{
    char axisChar = Dimensions <= 3u ? 'x' : '0';

    os << "DataExtent (" << Dimensions << " dimensions)\n";
    for (size_t d = 0u; d < Dimensions; ++d)
    {
        os << "    " << static_cast<char>(axisChar + d)
            << ": " << dataExtent[2u * d] << "; " << dataExtent[2u * d + 1u] << "\n";
    }

    return os;
}
