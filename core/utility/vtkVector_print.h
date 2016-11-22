#pragma once

#include <iosfwd>

#include <core/core_api.h>


class vtkVector2d;
class vtkVector3d;
class vtkVector2f;
class vtkVector3f;
class vtkVector2i;
class vtkVector3i;


CORE_API std::ostream & operator<<(std::ostream & os, const vtkVector2d & vector);
CORE_API std::ostream & operator<<(std::ostream & os, const vtkVector3d & vector);
CORE_API std::ostream & operator<<(std::ostream & os, const vtkVector2f & vector);
CORE_API std::ostream & operator<<(std::ostream & os, const vtkVector3f & vector);
CORE_API std::ostream & operator<<(std::ostream & os, const vtkVector2i & vector);
CORE_API std::ostream & operator<<(std::ostream & os, const vtkVector3i & vector);
