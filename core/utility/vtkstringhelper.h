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

#include <core/core_api.h>


class QString;
class vtkAbstractArray;
class vtkCharArray;
template<typename T> class vtkSmartPointer;


vtkSmartPointer<vtkCharArray> CORE_API qstringToVtkArray(const QString & string);
void CORE_API qstringToVtkArray(const QString & string, vtkCharArray & array);
QString CORE_API vtkArrayToQString(vtkAbstractArray & data);

/**
 * Check if the string is accepted by strange implementations inside VTK.
 *
 * VTK XML reader/writer use functions such as ::isspace for serialization/deserialization.
 * This function takes an int parameter, but it is undefined if the character is not representable
 * as unsigned char and it is not EOF (-1).
 * In debug libraries of MSVC, this functions contains a check as assert(-1 <= c <= 255) for all
 * chars in the string. Thus, some UTF-8 encoded characters, such as the degree sign will trigger
 * an assertion in MSVC.
 *
 * This is really, really stupid and apparently the only thing that prevents us from passing around
 * UTF-8 encoded string in VTK.
 */
bool CORE_API isStringIsAcceptable(const QString & string);
bool CORE_API isStringIsAcceptable(const char * const string, size_t length = 0);
