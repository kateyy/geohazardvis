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

#include <functional>
#include <vector>

#include <gui/gui_api.h>


class QDockWidget;
class QSettings;
class QString;
class DataSetHandler;
class DataMapping;
class MainWindow;


class GUI_API GuiPluginInterface
{
public:
    explicit GuiPluginInterface(
        MainWindow & mainWindow,
        DataMapping & dataMapping);
    virtual ~GuiPluginInterface();

    /** Adds a dock widget to the applications main window and a associated main menu entry to show the widget. */
    void addWidget(QDockWidget * widget);
    void removeWidget(QDockWidget * widget);

    /** Read/Write access to plugin settings stored with user defined application settings.
      * Such settings should always be accessed via these function, to ensure some consistency. */

    void readSettings(const std::function<void(const QSettings & settings)> & func);
    void readSettings(const QString & group, const std::function<void(const QSettings & settings)> & func);
    void readWriteSettings(const std::function<void(QSettings & settings)> & func);
    void readWriteSettings(const QString & group, const std::function<void(QSettings & settings)> & func);

    DataSetHandler & dataSetHandler() const;
    DataMapping & dataMapping() const;

    GuiPluginInterface(const GuiPluginInterface & other);
    friend void swap(GuiPluginInterface & lhs, GuiPluginInterface & rhs);
    GuiPluginInterface & operator=(GuiPluginInterface other);
    GuiPluginInterface(GuiPluginInterface && other);

private:
    GuiPluginInterface();

private:
    MainWindow * m_mainWindow;
    DataMapping * m_dataMapping;
    std::vector<QDockWidget *> m_widgets;
};
