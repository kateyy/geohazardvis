#pragma once

#include <ostream>

#include <core/utility/DataExtent.h>


template<typename T, size_t Dimensions>
std::ostream & operator<<(std::ostream & os, const DataExtent<T, Dimensions> & dataExtent);


#include "DataExtent_print.hpp"
