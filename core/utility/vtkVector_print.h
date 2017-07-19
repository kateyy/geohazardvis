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
