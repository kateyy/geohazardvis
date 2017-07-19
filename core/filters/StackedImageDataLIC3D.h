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

#include <vtkSimpleImageToImageFilter.h>

#include <core/core_api.h>


class NoiseImageSource;
class vtkRenderWindow;


class CORE_API StackedImageDataLIC3D : public vtkSimpleImageToImageFilter
{
public:
    static StackedImageDataLIC3D * New();
    vtkTypeMacro(StackedImageDataLIC3D, vtkSimpleImageToImageFilter);

protected:
    StackedImageDataLIC3D();
    ~StackedImageDataLIC3D() override;

    void SimpleExecute(vtkImageData * input, vtkImageData * output) override;

private:
    StackedImageDataLIC3D(const StackedImageDataLIC3D &) = delete;
    void operator=(const StackedImageDataLIC3D &) = delete;

    void initialize();

private:
    bool m_isInitialized;

    NoiseImageSource * m_noiseImage;
    vtkRenderWindow * m_glContext;
};