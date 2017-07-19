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

#include <gui/rendering_interaction/PickerHighlighterInteractorObserver.h>

#include <cassert>

#include <vtkCommand.h>
#include <vtkObjectFactory.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVector.h>

#include <gui/rendering_interaction/Highlighter.h>
#include <gui/rendering_interaction/Picker.h>


vtkStandardNewMacro(PickerHighlighterInteractorObserver);


PickerHighlighterInteractorObserver::PickerHighlighterInteractorObserver()
    : vtkInteractorObserver()
    , m_callbackTag{ 0u }
    , m_mouseMoved{ false }
    , m_pickOnMouseMove{ false }
    , m_picker{ std::make_unique<Picker>() }
    , m_highlighter{ std::make_unique<Highlighter>() }
{
    connect(m_highlighter.get(), &Highlighter::geometryChanged,
            this, &PickerHighlighterInteractorObserver::geometryChanged);
}

PickerHighlighterInteractorObserver::~PickerHighlighterInteractorObserver() = default;

void PickerHighlighterInteractorObserver::SetEnabled(int enabling)
{
    Superclass::SetEnabled(enabling);

    if (enabling != 0 && !this->Interactor)
    {
        vtkErrorMacro(<< "The interactor must be set prior to enabling/disabling the PickerHighlighterInteractorObserver");
        return;
    }

    if (enabling == 0)
    {
        // make sure not to have an invalid info text somewhere, if we don't update anymore
        m_picker->resetInformation();
        emit pickedInfoChanged("");
    }

    this->Enabled = enabling;
}

void PickerHighlighterInteractorObserver::SetInteractor(vtkRenderWindowInteractor * interactor)
{
    if (this->Interactor == interactor)
    {
        return;
    }

    if (this->Interactor)
    {
        this->Interactor->RemoveObserver(this->m_callbackTag);
        m_highlighter->clear();
    }

    if (interactor)
    {
        this->m_callbackTag = interactor->AddObserver(vtkCommand::AnyEvent, this, &PickerHighlighterInteractorObserver::EventCallback);
    }

    Superclass::SetInteractor(interactor);
}

Picker & PickerHighlighterInteractorObserver::picker()
{
    return *m_picker;
}

const Picker & PickerHighlighterInteractorObserver::picker() const
{
    return *m_picker;
}

Highlighter & PickerHighlighterInteractorObserver::highlighter()
{
    return *m_highlighter;
}

const Highlighter & PickerHighlighterInteractorObserver::highlighter() const
{
    return *m_highlighter;
}

bool PickerHighlighterInteractorObserver::picksOnMouseMove() const
{
    return m_pickOnMouseMove;
}

void PickerHighlighterInteractorObserver::setPickOnMouseMove(bool doPick)
{
    m_pickOnMouseMove = doPick;
}

const QString & PickerHighlighterInteractorObserver::pickedInfo() const
{
    return picker().pickedObjectInfoString();
}

void PickerHighlighterInteractorObserver::requestPickedInfoUpdate()
{
    if (!this->Enabled)
    {
        return;
    }

    pick();
}

void PickerHighlighterInteractorObserver::EventCallback(vtkObject * /*subject*/, unsigned long eventId, void * /*userData*/)
{
    if (!this->Enabled)
    {
        return;
    }

    switch (eventId)
    {
    case vtkCommand::LeftButtonPressEvent:
        m_mouseMoved = false;
        break;
    case vtkCommand::MouseMoveEvent:
        m_mouseMoved = true;
        if (m_pickOnMouseMove)
        {
            pick();
        }
        break;
    case vtkCommand::LeftButtonReleaseEvent:
        if (!m_mouseMoved)
        {
            pick();
            highlight();
        }

        break;
    }
}

void PickerHighlighterInteractorObserver::pick()
{
    if (!this->Interactor)
    {
        return;
    }

    vtkVector2i clickPos;
    this->Interactor->GetEventPosition(clickPos.GetData());
    auto renderer = this->Interactor->FindPokedRenderer(clickPos[0], clickPos[1]);
    assert(renderer);

    if (!renderer)
    {
        return;
    }

    m_picker->pick(clickPos, *renderer);

    emit pickedInfoChanged(m_picker->pickedObjectInfoString());
}

void PickerHighlighterInteractorObserver::highlight()
{
    vtkVector2i clickPos;
    this->Interactor->GetEventPosition(clickPos.GetData());
    auto renderer = this->Interactor->FindPokedRenderer(clickPos[0], clickPos[1]);
    assert(renderer);

    if (!renderer)
    {
        return;
    }

    m_highlighter->setRenderer(renderer);
    m_highlighter->setTarget(m_picker->pickedObjectInfo());

    emit dataPicked(m_picker->pickedObjectInfo());
}
