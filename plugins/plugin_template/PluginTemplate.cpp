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

#include "PluginTemplate.h"

#include <utility>

#include <QDockWidget>
#include <QLineEdit>
#include <QSettings>

#include <gui/plugin/GuiPluginInterface.h>


namespace
{
const char settingsPath[] = "PluginTemplate/EditText";
}


PluginTemplate::PluginTemplate(const QString & name, const QString & description, const QString & vendor, const QString & version, GuiPluginInterface && pluginInterface)
    : GuiPlugin(name, description, vendor, version, std::forward<GuiPluginInterface>(pluginInterface))
    , m_dockWidget(std::make_unique<QDockWidget>())
{
    QString text;
    m_pluginInterface.readSettings([&text] (const QSettings & settings) {
        text = settings.value(settingsPath, "Hello World").toString();
    });

    auto widget = new QWidget();
    m_dockWidget->setWidget(widget);
    m_dockWidget->setWindowTitle(name);

    m_lineEdit = new QLineEdit(text, widget);

    m_pluginInterface.addWidget(m_dockWidget.get());
}

PluginTemplate::~PluginTemplate()
{
    m_pluginInterface.removeWidget(m_dockWidget.get());

    m_pluginInterface.readWriteSettings([this] (QSettings & settings) {
        settings.setValue(settingsPath, m_lineEdit->text());
    });
}
