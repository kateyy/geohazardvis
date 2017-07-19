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

#include <QString>

#include <gui/gui_api.h>
#include <gui/plugin/GuiPluginInterface.h>


class GUI_API GuiPlugin
{
public:
    explicit GuiPlugin(
        const QString & name,
        const QString & description,
        const QString & vendor,
        const QString & version,
        GuiPluginInterface && pluginInterface);

    virtual ~GuiPlugin();

    const QString & name() const;
    const QString & description() const;
    const QString & vendor() const;
    const QString & version() const;

protected:
    QString m_name;
    QString m_description;
    QString m_vendor;
    QString m_version;

    GuiPluginInterface m_pluginInterface;
};
