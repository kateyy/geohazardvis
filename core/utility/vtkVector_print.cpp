#include "vtkVector_print.h"

#include <ostream>

#include <vtkVector.h>


std::ostream & operator<<(std::ostream & os, const vtkVector2d & vector)
{
    os << vector.GetX() << ":" << vector.GetY();
    return os;
}

std::ostream & operator<<(std::ostream & os, const vtkVector3d & vector)
{
    os << vector.GetX() << ":" << vector.GetY() << ":" << vector.GetZ();
    return os;
}

std::ostream & operator<<(std::ostream & os, const vtkVector2f & vector)
{
    os << vector.GetX() << ":" << vector.GetY();
    return os;
}

std::ostream & operator<<(std::ostream & os, const vtkVector3f & vector)
{
    os << vector.GetX() << ":" << vector.GetY() << ":" << vector.GetZ();
    return os;
}

std::ostream & operator<<(std::ostream & os, const vtkVector2i & vector)
{
    os << vector.GetX() << ":" << vector.GetY();
    return os;
}

std::ostream & operator<<(std::ostream & os, const vtkVector3i & vector)
{
    os << vector.GetX() << ":" << vector.GetY() << ":" << vector.GetZ();
    return os;
}
