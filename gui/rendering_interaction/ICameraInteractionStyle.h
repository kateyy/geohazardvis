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

#include <core/types.h>

#include <gui/gui_api.h>


class vtkCamera;


/** Interface extensions for the vtkInteractorStyle classes 
  * Implement camera movements and reset while ensuring the constraints defined by a specific interaction style. */
class GUI_API ICameraInteractionStyle
{
public:
    virtual void resetCameraToDefault(vtkCamera & camera) = 0;
    virtual void moveCameraTo(
        const VisualizationSelection & selection,
        bool overTime = true) = 0;

    virtual ~ICameraInteractionStyle() = default;
};
