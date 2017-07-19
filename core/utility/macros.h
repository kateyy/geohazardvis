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


#if defined(NDEBUG)
#define DEBUG_ONLY(S)
#else
#define DEBUG_ONLY(S) S
#endif


// https://stackoverflow.com/questions/471935/user-warnings-on-msvc-and-gcc
// Use: #pragma message WARN("My message")
#define STRINGISE_IMPL(x) #x
#define STRINGISE(x) STRINGISE_IMPL(x)
#if _MSC_VER
#   define FILE_LINE_LINK __FILE__ "(" STRINGISE(__LINE__) ") : "
#   define WARN(exp) (FILE_LINE_LINK "WARNING: " exp)
#else//__GNUC__ - may need other defines for different compilers
#   define WARN(exp) ("WARNING: " exp)
#endif


// Boolean expression checking if VTK version is at least MAJOR.MINOR.BUILD
// This requires to include vtkVersionMacros.h BEFORE this header.
#ifdef VTK_MAJOR_VERSION
#define VTK_CHECK_VERSION(MAJOR, MINOR, BUILD) \
    (VTK_MAJOR_VERSION > MAJOR \
    || (VTK_MAJOR_VERSION == MAJOR && VTK_MINOR_VERSION > MINOR) \
    || (VTK_MAJOR_VERSION == MAJOR && VTK_MINOR_VERSION == MINOR && VTK_BUILD_VERSION >= BUILD))
#endif
