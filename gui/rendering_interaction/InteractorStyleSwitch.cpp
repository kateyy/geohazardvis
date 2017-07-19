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

#include "InteractorStyleSwitch.h"

#include <cassert>

#include <vtkObjectFactory.h>
#include <vtkRenderer.h>


vtkStandardNewMacro(InteractorStyleSwitch);

InteractorStyleSwitch::InteractorStyleSwitch()
    : Superclass()
    , m_currentStyle{ nullptr }
{
}

InteractorStyleSwitch::~InteractorStyleSwitch() = default;

void InteractorStyleSwitch::addStyle(const std::string & name, vtkInteractorStyle * interactorStyle)
{
    assert(m_namedStyles.find(name) == m_namedStyles.end());

    m_namedStyles.emplace(name, interactorStyle);

    interactorStyle->SetDefaultRenderer(GetDefaultRenderer());
    interactorStyle->SetCurrentRenderer(GetCurrentRenderer());

    if (!m_currentStyle)
    {
        setCurrentStyle(name);
    }

    styleAddedEvent(interactorStyle);
}

void InteractorStyleSwitch::setCurrentStyle(const std::string & name)
{
    if (m_currentStyleName == name)
    {
        return;
    }

    auto styleIt = m_namedStyles.find(name);
    assert(styleIt != m_namedStyles.end());


    if (m_currentStyle)
    {
        m_currentStyle->SetInteractor(nullptr);
    }

    m_currentStyleName = name;
    m_currentStyle = styleIt->second;

    m_currentStyle->SetInteractor(GetInteractor());

    currentStyleChangedEvent();

    this->InvokeEvent(StyleChangedEvent, nullptr);
}

const std::string & InteractorStyleSwitch::currentStyleName() const
{
    return m_currentStyleName;
}

vtkInteractorStyle * InteractorStyleSwitch::currentStyle()
{
    return m_currentStyle;
}

void InteractorStyleSwitch::SetInteractor(vtkRenderWindowInteractor * interactor)
{
    vtkInteractorStyle::SetInteractor(interactor);

    if (m_currentStyle)
    {
        m_currentStyle->SetInteractor(interactor);
    }
}

void InteractorStyleSwitch::SetAutoAdjustCameraClippingRange(int value)
{
    vtkInteractorStyle::SetAutoAdjustCameraClippingRange(value);

    for (auto & it : m_namedStyles)
    {
        it.second->SetAutoAdjustCameraClippingRange(value);
    }
}

void InteractorStyleSwitch::SetDefaultRenderer(vtkRenderer * renderer)
{
    vtkInteractorStyle::SetDefaultRenderer(renderer);

    for (auto & it : m_namedStyles)
    {
        it.second->SetDefaultRenderer(renderer);
    }
}

void InteractorStyleSwitch::SetCurrentRenderer(vtkRenderer * renderer)
{
    vtkInteractorStyle::SetCurrentRenderer(renderer);

    for (auto & it : m_namedStyles)
    {
        it.second->SetCurrentRenderer(renderer);
    }
}

void InteractorStyleSwitch::styleAddedEvent(vtkInteractorStyle * /*interactorStyle*/)
{
}

void InteractorStyleSwitch::currentStyleChangedEvent()
{
}

const std::unordered_map<std::string, vtkSmartPointer<vtkInteractorStyle>> InteractorStyleSwitch::namedStyles() const
{
    return m_namedStyles;
}
