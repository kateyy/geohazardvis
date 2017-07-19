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

#include "GuiPluginLibrary.h"

#include <utility>


GuiPluginLibrary::GuiPluginLibrary(const QString & filePath)
    : m_filePath(filePath)
    , m_initPtr(nullptr)
    , m_releasePtr(nullptr)
{
}

GuiPluginLibrary::~GuiPluginLibrary() = default;

const QString & GuiPluginLibrary::filePath() const
{
    return m_filePath;
}

bool GuiPluginLibrary::isValid() const
{
    return (m_initPtr && m_pluginPtr && m_releasePtr);
}

void GuiPluginLibrary::initialize(GuiPluginInterface && pluginInterface)
{
    if (m_initPtr != nullptr)
    {
        (*m_initPtr)(std::forward<GuiPluginInterface>(pluginInterface));
    }
}

void GuiPluginLibrary::release()
{
    if (m_releasePtr != nullptr)
    {
        (*m_releasePtr)();
    }
}

GuiPlugin * GuiPluginLibrary::plugin() const
{
    if (m_pluginPtr != nullptr)
    {
        return (*m_pluginPtr)();
    }

    return nullptr;
}
