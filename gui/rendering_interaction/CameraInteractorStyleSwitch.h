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

#include <gui/rendering_interaction/InteractorStyleSwitch.h>
#include <gui/rendering_interaction/ICameraInteractionStyle.h>


class GUI_API CameraInteractorStyleSwitch : public InteractorStyleSwitch, virtual public ICameraInteractionStyle
{
public:
    static CameraInteractorStyleSwitch * New();
    vtkTypeMacro(CameraInteractorStyleSwitch, InteractorStyleSwitch);

    void resetCameraToDefault(vtkCamera & camera) override;
    void moveCameraTo(const VisualizationSelection & selection, bool overTime = true) override;
    
protected:
    CameraInteractorStyleSwitch();
    ~CameraInteractorStyleSwitch() override;

    void currentStyleChangedEvent() override;

private:
    ICameraInteractionStyle * m_currentCameraStyle;

private:
    CameraInteractorStyleSwitch(const CameraInteractorStyleSwitch &) = delete;
    void operator=(const CameraInteractorStyleSwitch &) = delete;
};
