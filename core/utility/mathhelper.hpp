#include <core/utility/mathhelper.h>


namespace mathhelper
{

template <typename T> char sgn(const T & value)
{
    return static_cast<char>((T(0) <= value) - (value < T(0)));
}

}
