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

#include <QObject>

#include <vtkInteractorObserver.h>

#include <core/types.h>
#include <gui/gui_api.h>


class Highlighter;
class Picker;


class GUI_API PickerHighlighterInteractorObserver : public QObject, public vtkInteractorObserver
{
    Q_OBJECT

public:
    static PickerHighlighterInteractorObserver * New();
    vtkTypeMacro(PickerHighlighterInteractorObserver, vtkInteractorObserver);

    void SetEnabled(int enabling) override;
    void SetInteractor(vtkRenderWindowInteractor * interactor) override;

    Picker & picker();
    const Picker & picker() const;
    Highlighter & highlighter();
    const Highlighter & highlighter() const;

    bool picksOnMouseMove() const;
    void setPickOnMouseMove(bool doPick);

    const QString & pickedInfo() const;
    void requestPickedInfoUpdate();

signals:
    void pickedInfoChanged(const QString & infoText);
    void dataPicked(const VisualizationSelection & selection);
    void geometryChanged();

protected:
    PickerHighlighterInteractorObserver();
    ~PickerHighlighterInteractorObserver() override;

    void EventCallback(vtkObject * subject, unsigned long eventId, void * userData);

    void pick();
    void highlight();

private:
    unsigned long m_callbackTag;
    bool m_mouseMoved;
    bool m_pickOnMouseMove;

    std::unique_ptr<Picker> m_picker;
    std::unique_ptr<Highlighter> m_highlighter;

private:
    PickerHighlighterInteractorObserver(const PickerHighlighterInteractorObserver &) = delete;
    void operator=(const PickerHighlighterInteractorObserver &) = delete;
};
