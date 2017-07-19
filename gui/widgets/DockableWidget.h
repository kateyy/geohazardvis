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

#include <QWidget>

#include <gui/gui_api.h>


class QDockWidget;


class GUI_API DockableWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DockableWidget(QWidget * parent = nullptr, Qt::WindowFlags f = {});
    ~DockableWidget() override;

    /** @return a QDockWidget, that contains the widget, to be embedded in QMainWindows
      * Creates the QDockWidget instance if required. */
    QDockWidget * dockWidgetParent();
    QDockWidget * dockWidgetParentOrNullptr();
    bool hasDockWidgetParent() const;

    bool isClosed() const;

signals:
    void closed();

protected:
    /** Call this in subclass destructors to make sure that closed() is signaled before the object 
      * gets invalidated.
      * This is only required to fix cases where the widget is never shown. */
    void signalClosing();

    bool eventFilter(QObject * obj, QEvent * ev) override;
    void closeEvent(QCloseEvent * event) override;
    
private:
    QDockWidget * m_dockWidgetParent;

    bool m_closingReported;

private:
    Q_DISABLE_COPY(DockableWidget)
};
