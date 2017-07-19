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

#include "VersionInfo.h"

#include <map>

#include <vtkVersionMacros.h>

#include "vcs_commit_info.h"


const VersionInfo & VersionInfo::projectInfo()
{
    return versionInfoFor(QString());
}

const VersionInfo & VersionInfo::versionInfoFor(const QString & thirdPartyName)
{
    static const std::map<QString, VersionInfo> infos = {
        { QString(), VersionInfo(
            PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH,
            PROJECT_MAINTAINER_EMAIL,
            GIT_SHA1, GIT_REV, GIT_DATE)
        },
        { "VTK", VersionInfo(
            VTK_MAJOR_VERSION, VTK_MINOR_VERSION, VTK_BUILD_VERSION,
            "",
            VTK_GIT_SHA1, VTK_GIT_REV, VTK_GIT_DATE)
        }
    };
    return infos.at(thirdPartyName);
}

const QStringList & VersionInfo::supportedThirdParties()
{
    static const QStringList list = { "VTK" };
    return list;
}

VersionInfo::VersionInfo(int major, int minor, int patch, const char * maintainerEmail, const char * sha1, const char * rev, const char * date)
    : major{ major }
    , minor{ minor }
    , patch{ patch }
    , maintainerEmail{ QString::fromLatin1(maintainerEmail) }
    , gitSHA1{ QString::fromLatin1(sha1) }
    , gitRevision{ QString::fromLatin1(rev) }
    , gitCommitDateString{ QString::fromLatin1(date) }
    , gitCommitDate{ QDateTime::fromString(QString::fromLatin1(date), Qt::ISODate) }
{
}
