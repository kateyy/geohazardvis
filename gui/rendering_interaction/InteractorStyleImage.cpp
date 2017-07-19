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

#include "InteractorStyleImage.h"

#include <array>

#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>


vtkStandardNewMacro(InteractorStyleImage);

InteractorStyleImage::InteractorStyleImage()
    : Superclass()
    , m_mouseMoved{ false }
    , m_mouseButtonDown{ false }
{
}

InteractorStyleImage::~InteractorStyleImage() = default;

void InteractorStyleImage::OnMouseMove()
{
    if (m_mouseButtonDown && !m_mouseMoved)
    {
        GrabFocus(EventCallbackCommand);

        m_mouseMoved = true;
    }

    Superclass::OnMouseMove();
}

void InteractorStyleImage::OnLeftButtonDown()
{
    std::array<int, 2> eventPos;
    GetInteractor()->GetEventPosition(eventPos.data());
    FindPokedRenderer(eventPos[0], eventPos[1]);

    if (!GetCurrentRenderer())
    {
        return;
    }

    m_mouseMoved = false;
    m_mouseButtonDown = true;

    StartPan();
}

void InteractorStyleImage::OnLeftButtonUp()
{
    m_mouseMoved = false;
    m_mouseButtonDown = false;

    Superclass::OnLeftButtonUp();
}

void InteractorStyleImage::OnMiddleButtonDown()
{
    std::array<int, 2> eventPos;
    GetInteractor()->GetEventPosition(eventPos.data());
    FindPokedRenderer(eventPos[0], eventPos[1]);

    if (!GetCurrentRenderer())
    {
        return;
    }

    StartDolly();
}

void InteractorStyleImage::OnMiddleButtonUp()
{
    switch (State)
    {
    case VTKIS_DOLLY:
        EndDolly();
        if (Interactor)
        {
            ReleaseFocus();
        }
        break;
    default:
        Superclass::OnMiddleButtonUp();
    }
}

void InteractorStyleImage::OnRightButtonDown()
{
    OnLeftButtonDown();
}

void InteractorStyleImage::OnRightButtonUp()
{
    OnLeftButtonUp();
}

void InteractorStyleImage::OnChar()
{
    // disable magic keys for now
}

void InteractorStyleImage::resetCameraToDefault(vtkCamera & camera)
{
    camera.SetViewUp(0, 1, 0);
    camera.SetFocalPoint(0, 0, 0);
    camera.SetPosition(0, 0, 1);
    camera.ParallelProjectionOn();
}

void InteractorStyleImage::moveCameraTo(const VisualizationSelection & /*selection*/, bool /*overTime*/)
{
    // TODO
}
