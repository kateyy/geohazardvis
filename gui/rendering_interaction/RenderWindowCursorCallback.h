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

#include <QObject>

#include <vtkWeakPointer.h>

#include <gui/gui_api.h>


class QWidget;
class vtkObject;
class vtkRenderWindow;


class GUI_API RenderWindowCursorCallback : public QObject
{
public:
    explicit RenderWindowCursorCallback(QObject * parent = nullptr);
    ~RenderWindowCursorCallback() override;

    void setRenderWindow(vtkRenderWindow * renderWindow);
    void setQWidget(QWidget * widget);

    static Qt::CursorShape vtkToQtCursor(int vtkCursorId, bool holdingMouse);

private:
    bool eventFilter(QObject * object, QEvent * event) override;

    void callback(vtkObject *, unsigned long, void *);

    void updateQtWidgetCursor(bool forceUpdate = false);

private:
    vtkWeakPointer<vtkRenderWindow> m_renderWindow;
    unsigned long m_observerTag;
    QWidget * m_qWidget;
    int m_currentVtkCursor;

    bool m_holdingMouse;

private:
    Q_DISABLE_COPY(RenderWindowCursorCallback)
};
