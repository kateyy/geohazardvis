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

#include "RenderWindowCursorCallback.h"

#include <cassert>

#include <QMouseEvent>
#include <QWidget>

#include <vtkCommand.h>
#include <vtkRenderWindow.h>

#include <core/utility/macros.h>


RenderWindowCursorCallback::RenderWindowCursorCallback(QObject * parent)
    : QObject(parent)
    , m_renderWindow{ nullptr }
    , m_observerTag{ 0 }
    , m_qWidget{ nullptr }
    , m_currentVtkCursor{ -1 }
    , m_holdingMouse{ false }
{
}

RenderWindowCursorCallback::~RenderWindowCursorCallback()
{
    if (m_renderWindow)
    {
        m_renderWindow->RemoveObserver(m_observerTag);
    }
}

void RenderWindowCursorCallback::setRenderWindow(vtkRenderWindow * renderWindow)
{
    if (m_renderWindow)
    {
        m_renderWindow->RemoveObserver(m_observerTag);
    }

    m_renderWindow = renderWindow;

    if (m_renderWindow)
    {
        m_observerTag = m_renderWindow->AddObserver(vtkCommand::ModifiedEvent, 
            this, &RenderWindowCursorCallback::callback);
    }
}

void RenderWindowCursorCallback::setQWidget(QWidget * widget)
{
    m_qWidget = widget;

    if (m_qWidget)
    {
        m_qWidget->installEventFilter(this);
    }
}

Qt::CursorShape RenderWindowCursorCallback::vtkToQtCursor(int vtkCursorId, bool holdingMouse)
{
    switch (vtkCursorId)
    {
    case VTK_CURSOR_SIZENE:
    case VTK_CURSOR_SIZESW: return Qt::SizeBDiagCursor;
    case VTK_CURSOR_SIZENW:
    case VTK_CURSOR_SIZESE: return Qt::SizeFDiagCursor;
    case VTK_CURSOR_SIZENS: return Qt::SizeVerCursor;
    case VTK_CURSOR_SIZEWE: return Qt::SizeHorCursor;
    case VTK_CURSOR_SIZEALL: /*return Qt::SizeAllCursor;*/
                             // use the "hand" cursor instead to move widgets
    case VTK_CURSOR_HAND: // toggle between open/closed hand
        return holdingMouse
            ? Qt::ClosedHandCursor
            : Qt::OpenHandCursor;
    case VTK_CURSOR_CROSSHAIR: return Qt::CrossCursor;

    case VTK_CURSOR_DEFAULT:
    case VTK_CURSOR_ARROW:
    default:
        return Qt::ArrowCursor;
    }
}

bool RenderWindowCursorCallback::eventFilter(QObject * object, QEvent * event)
{
    if (object != m_qWidget)
    {
        object->removeEventFilter(this);
        return false;
    }

    switch (event->type())
    {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
        break;
    default:
        return false;
    }

    auto mouseEvent = static_cast<QMouseEvent &>(*event);
    bool holding = (mouseEvent.buttons() & (Qt::LeftButton | Qt::MiddleButton)) != Qt::NoButton;
    if (holding != m_holdingMouse)
    {
        m_holdingMouse = holding;
        updateQtWidgetCursor(true);
    }

    return false;
}

void RenderWindowCursorCallback::callback(vtkObject * DEBUG_ONLY(source), unsigned long, void *)
{
    assert(source == m_renderWindow.Get());

    updateQtWidgetCursor();
}

void RenderWindowCursorCallback::updateQtWidgetCursor(bool forceUpdate)
{
    if (!m_qWidget || !m_renderWindow)
    {
        return;
    }

    const int newVtkCursor = m_renderWindow->GetCurrentCursor();

    if (!forceUpdate && (newVtkCursor == m_currentVtkCursor))
    {
        return;
    }

    m_currentVtkCursor = newVtkCursor;

    m_qWidget->setCursor(vtkToQtCursor(m_currentVtkCursor, m_holdingMouse));
}
