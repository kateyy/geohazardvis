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

#include "CameraInteractorStyleSwitch.h"

#include <vtkObjectFactory.h>


vtkStandardNewMacro(CameraInteractorStyleSwitch);


CameraInteractorStyleSwitch::CameraInteractorStyleSwitch()
    : InteractorStyleSwitch()
    , m_currentCameraStyle{ nullptr }
{
}

CameraInteractorStyleSwitch::~CameraInteractorStyleSwitch() = default;

void CameraInteractorStyleSwitch::resetCameraToDefault(vtkCamera & camera)
{
    if (!m_currentCameraStyle)
    {
        return;
    }

    m_currentCameraStyle->resetCameraToDefault(camera);
}

void CameraInteractorStyleSwitch::moveCameraTo(const VisualizationSelection & selection, bool overTime)
{
    if (!m_currentCameraStyle)
    {
        return;
    }

    m_currentCameraStyle->moveCameraTo(selection, overTime);
}

void CameraInteractorStyleSwitch::currentStyleChangedEvent()
{
    m_currentCameraStyle = dynamic_cast<ICameraInteractionStyle *>(currentStyle());
}
