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

#include <core/filters/ImagePlaneWidget.h>

#include <cassert>

#include <vtkActor.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataMapper.h>


vtkStandardNewMacro(ImagePlaneWidget);


ImagePlaneWidget::ImagePlaneWidget() = default;

ImagePlaneWidget::~ImagePlaneWidget() = default;

vtkActor * ImagePlaneWidget::GetTexturePlaneActor()
{
    assert(this->TexturePlaneActor);

    return this->TexturePlaneActor;
}

vtkPolyDataMapper * ImagePlaneWidget::GetTexturePlaneMapper()
{
    auto mapper = GetTexturePlaneActor()->GetMapper();

    assert(dynamic_cast<vtkPolyDataMapper *>(mapper));

    return static_cast<vtkPolyDataMapper *>(mapper);
}
