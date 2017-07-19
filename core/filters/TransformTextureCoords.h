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

#include <vtkTransformTextureCoords.h>

#include <core/core_api.h>


class CORE_API TransformTextureCoords : public vtkTransformTextureCoords
{
public:
    vtkTypeMacro(TransformTextureCoords, vtkTransformTextureCoords);
    static TransformTextureCoords * New();

protected:
    TransformTextureCoords() = default;

    int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
    TransformTextureCoords(const TransformTextureCoords&) = delete;
    void operator=(const TransformTextureCoords&) = delete;
};
