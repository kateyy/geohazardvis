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

#include <string>
#include <unordered_map>

#include <vtkCommand.h>
#include <vtkInteractorStyle.h>
#include <vtkSmartPointer.h>

#include <gui/gui_api.h>


class GUI_API InteractorStyleSwitch : public vtkInteractorStyle
{
public:
    static InteractorStyleSwitch * New();
    vtkTypeMacro(InteractorStyleSwitch, vtkInteractorStyle);

    enum
    {
        StyleChangedEvent = vtkCommand::UserEvent + 1
    };

    void addStyle(const std::string & name, vtkInteractorStyle * interactorStyle);
    void setCurrentStyle(const std::string & name);
    const std::string & currentStyleName() const;
    vtkInteractorStyle * currentStyle();

    void SetInteractor(vtkRenderWindowInteractor * interactor) override;
    void SetAutoAdjustCameraClippingRange(int value) override;
    void SetDefaultRenderer(vtkRenderer * renderer) override;
    void SetCurrentRenderer(vtkRenderer * renderer) override;

protected:
    InteractorStyleSwitch();
    ~InteractorStyleSwitch() override;

    virtual void styleAddedEvent(vtkInteractorStyle * interactorStyle);
    virtual void currentStyleChangedEvent();
    const std::unordered_map<std::string, vtkSmartPointer<vtkInteractorStyle>> namedStyles() const;

private:
    std::unordered_map<std::string, vtkSmartPointer<vtkInteractorStyle>> m_namedStyles;
    std::string m_currentStyleName;
    vtkInteractorStyle * m_currentStyle;

public:
    InteractorStyleSwitch(const InteractorStyleSwitch&) = delete;
    void operator=(const InteractorStyleSwitch&) = delete;
};
