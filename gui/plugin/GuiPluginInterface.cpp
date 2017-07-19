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

#include "GuiPluginInterface.h"

#include <cassert>

#include <QAction>
#include <QDockWidget>
#include <QMenuBar>

#include <core/ApplicationSettings.h>
#include <gui/DataMapping.h>
#include <gui/MainWindow.h>


namespace
{
const QString & pluginsSettingsGroup()
{
    static const QString groupName = "plugins";
    return groupName;
}
}


GuiPluginInterface::GuiPluginInterface(MainWindow & mainWindow, DataMapping & dataMapping)
    : m_mainWindow{ &mainWindow }
    , m_dataMapping{ &dataMapping }
{
}

GuiPluginInterface::~GuiPluginInterface()
{
    for (auto * widget : m_widgets)
    {
        removeWidget(widget);
    }
}

void GuiPluginInterface::addWidget(QDockWidget * widget)
{
    assert(std::find(m_widgets.begin(), m_widgets.end(), widget) == m_widgets.end());
    if (std::find(m_widgets.begin(), m_widgets.end(), widget) != m_widgets.end())
    {
        return;
    }

    m_mainWindow->addDockWidget(Qt::RightDockWidgetArea, widget);
    widget->hide();

    m_mainWindow->addPluginMenuAction(widget->toggleViewAction());

    m_widgets.push_back(widget);
}

void GuiPluginInterface::removeWidget(QDockWidget * widget)
{
    assert(widget);
    auto it = std::find(m_widgets.begin(), m_widgets.end(), widget);

    if (it == m_widgets.end())
    {
        assert(false);
        return;
    }

    m_mainWindow->removePluginMenuAction(widget->toggleViewAction());

    // removeDockWidget creates a placeholder item, which is identified by the widget's object name
    // The object name (QString) might be set by the ui-file in the plugin's resources. After unloading
    // the plugin, this QString object is not available anymore, which leads to invalid memory access.
    // [Anyway, why is this not handled correctly by Qt?]
    widget->setObjectName(QString());
    m_mainWindow->removeDockWidget(widget);

    m_widgets.erase(it);
}

void GuiPluginInterface::readSettings(const std::function<void(const QSettings & settings)> & func)
{
    ApplicationSettings settings;
    settings.beginGroup(pluginsSettingsGroup());

    func(settings);
}

void GuiPluginInterface::readSettings(const QString & group, const std::function<void(const QSettings & settings)> & func)
{
    ApplicationSettings settings;
    settings.beginGroup(pluginsSettingsGroup());
    settings.beginGroup(group);

    func(settings);
}

void GuiPluginInterface::readWriteSettings(const std::function<void(QSettings & settings)> & func)
{
    ApplicationSettings settings;
    settings.beginGroup(pluginsSettingsGroup());

    func(settings);
}

void GuiPluginInterface::readWriteSettings(const QString & group, const std::function<void(QSettings & settings)> & func)
{
    ApplicationSettings settings;
    settings.beginGroup(pluginsSettingsGroup());
    settings.beginGroup(group);

    func(settings);
}

DataSetHandler & GuiPluginInterface::dataSetHandler() const
{
    return m_dataMapping->dataSetHandler();
}

DataMapping & GuiPluginInterface::dataMapping() const
{
    return *m_dataMapping;
}

GuiPluginInterface::GuiPluginInterface(const GuiPluginInterface & other)
    : m_mainWindow{ other.m_mainWindow }
    , m_dataMapping{ other.m_dataMapping }
    , m_widgets{ other.m_widgets }
{
}

void swap(GuiPluginInterface & lhs, GuiPluginInterface & rhs)
{
    // https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom

    using std::swap;

    swap(lhs.m_mainWindow, rhs.m_mainWindow);
    swap(lhs.m_dataMapping, rhs.m_dataMapping);
    swap(lhs.m_widgets, rhs.m_widgets);
}

GuiPluginInterface::GuiPluginInterface()
    : m_mainWindow{ nullptr }
    , m_dataMapping{ nullptr }
{
}

GuiPluginInterface & GuiPluginInterface::operator=(GuiPluginInterface other)
{
    swap(*this, other);

    return *this;
}

GuiPluginInterface::GuiPluginInterface(GuiPluginInterface && other)
    : GuiPluginInterface()
{
    swap(*this, other);
}
