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

#include <QDateTime>
#include <QString>

#include <core/core_api.h>


class CORE_API VersionInfo
{
public:
    int major;
    int minor;
    int patch;
    QString maintainerEmail;
    QString gitSHA1;
    QString gitRevision;
    QString gitCommitDateString;
    QDateTime gitCommitDate;

    static const VersionInfo & projectInfo();
    static const VersionInfo & versionInfoFor(const QString & thirdPartyName);
    static const QStringList & supportedThirdParties();

private:
    VersionInfo(int major, int minor, int patch,
        const char * maintainerEmail,
        const char * sha1, const char * rev, const char * date);
};
