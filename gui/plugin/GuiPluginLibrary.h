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


class GuiPlugin;
class GuiPluginInterface;


class GuiPluginLibrary
{
public:
    using init_ptr = void(*)(GuiPluginInterface &&);
    using plugin_ptr = GuiPlugin * (*)();
    using release_ptr = void(*)();

    explicit GuiPluginLibrary(const QString & filePath);
    virtual ~GuiPluginLibrary();

    const QString & filePath() const;

    bool isValid() const;

    void initialize(GuiPluginInterface && pluginInterface);
    void release();

    GuiPlugin * plugin() const;

protected:
    QString m_filePath;
    init_ptr m_initPtr;
    release_ptr m_releasePtr;
    plugin_ptr m_pluginPtr;
};
