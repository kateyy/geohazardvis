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

#include <QSettings>

#include <core/core_api.h>


/**
QSettings subclass that accesses application settings from a default location.
*/
class CORE_API ApplicationSettings : public QSettings
{
public:
    explicit ApplicationSettings(QObject * parent = nullptr);
    ~ApplicationSettings() override;

    /** @return true, if there are any stored settings for the application.
      * Normally, this returns false only if the application never ran on the current system/user. */
    static bool settingsExist();

private:
    Q_DISABLE_COPY(ApplicationSettings)
};
