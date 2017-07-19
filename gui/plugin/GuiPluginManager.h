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
#include <vector>

#include <QStringList>


class GuiPlugin;
class GuiPluginInterface;
class GuiPluginLibrary;


/**
 * Plugin loading and management based on gloperate's plugin system
 *
 * @see https://github.com/cginternals/gloperate
*/
class GuiPluginManager
{
public:
    GuiPluginManager();

    virtual ~GuiPluginManager();

    /** Access the paths that will be searched for plugins */
    QStringList & searchPaths();
    const QStringList & searchPaths() const;

    /** Load all plugins found in the search paths */
    void scan(GuiPluginInterface pluginInterface);

    const std::vector<GuiPlugin *> & plugins() const;
    std::vector<GuiPluginLibrary *> pluginLibraries() const;

private:
    bool loadLibrary(const QString & filePath, GuiPluginInterface && pluginInterface);
    void unloadLibrary(std::unique_ptr<GuiPluginLibrary> library);

private:
    QStringList m_searchPaths;

    std::vector<std::unique_ptr<GuiPluginLibrary>> m_libraries;
    std::vector<GuiPlugin *> m_plugins;
};
