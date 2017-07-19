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

#include <memory>

#include <vtkInteractorStyleTerrain.h>

#include <gui/rendering_interaction/ICameraInteractionStyle.h>


class CameraDolly;


class GUI_API InteractorStyleTerrain : public vtkInteractorStyleTerrain, virtual public ICameraInteractionStyle
{
public:
    static InteractorStyleTerrain * New();
    vtkTypeMacro(InteractorStyleTerrain, vtkInteractorStyleTerrain);

    void OnMouseMove() override;
    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void OnMiddleButtonDown() override;
    void OnMiddleButtonUp() override;
    void OnRightButtonDown() override;
    void OnRightButtonUp() override;
    void OnMouseWheelForward() override;
    void OnMouseWheelBackward() override;

    void OnChar() override;

    void resetCameraToDefault(vtkCamera & camera) override;
    void moveCameraTo(const VisualizationSelection & selection, bool overTime = true) override;

protected:
    explicit InteractorStyleTerrain();
    ~InteractorStyleTerrain();

    void MouseWheelDolly(bool forward);

private:
    std::unique_ptr<CameraDolly> m_cameraDolly;

    bool m_mouseMoved;

private:
    InteractorStyleTerrain(const InteractorStyleTerrain &) = delete;
    void operator=(const InteractorStyleTerrain &) = delete;
};
