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

#include "GuiPlugin.h"

#include <utility>


GuiPlugin::GuiPlugin(
    const QString & name, const QString & description, const QString & vendor, const QString & version, GuiPluginInterface && pluginInterface)
    : m_name{ name }
    , m_description{ description }
    , m_vendor{ vendor }
    , m_version{ version }
    , m_pluginInterface{ std::forward<GuiPluginInterface>(pluginInterface) }
{
}

GuiPlugin::~GuiPlugin() = default;

const QString & GuiPlugin::name() const
{
    return m_name;
}

const QString & GuiPlugin::description() const
{
    return m_description;
}

const QString & GuiPlugin::vendor() const
{
    return m_vendor;
}

const QString & GuiPlugin::version() const
{
    return m_version;
}
