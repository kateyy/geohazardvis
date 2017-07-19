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
#include "third_party_info.h"


namespace
{
    QStringList l_thirdPartiesWithVersionInfo;
    QStringList l_otherThirdParties;
}


const VersionInfo & VersionInfo::projectInfo()
{
    return versionInfoFor(QString());
}

const VersionInfo & VersionInfo::versionInfoFor(const QString & thirdPartyName)
{
    static const std::map<QString, VersionInfo> infos = [] () {
        std::map<QString, VersionInfo> map = { { QString(), VersionInfo(
            PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH,
            PROJECT_MAINTAINER_EMAIL,
            GIT_SHA1, GIT_REV, GIT_DATE)
        }};
        for (const auto & info : thirdPartyInfos)
        {
            if (info.hasGitInfo)
            {
                l_thirdPartiesWithVersionInfo.push_back(info.name);
                map.emplace(info.name, VersionInfo(info.gitSha1, info.gitRev, info.gitDate));
            }
            else
            {
                l_otherThirdParties.push_back(info.name);
            }
        }
        auto & vtkInfo = map.at("VTK");
        vtkInfo.major = VTK_MAJOR_VERSION;
        vtkInfo.minor = VTK_MINOR_VERSION;
        vtkInfo.patch = VTK_BUILD_VERSION;
        return map;
    }();

    return infos.at(thirdPartyName);
}

const QStringList & VersionInfo::thirdPartiesWithVersionInfo()
{
    versionInfoFor({});
    return l_thirdPartiesWithVersionInfo;
}

const QStringList & VersionInfo::otherThirdParties()
{
    versionInfoFor({});
    return l_otherThirdParties;
}

VersionInfo::VersionInfo(
    int major, int minor, int patch,
    const QString & maintainerEmail,
    const QString & sha1, const QString & rev, const QString & date)
    : major{ major }
    , minor{ minor }
    , patch{ patch }
    , maintainerEmail{ maintainerEmail }
    , gitSHA1{ sha1 }
    , gitRevision{ rev }
    , gitCommitDateString{ date }
    , gitCommitDate{ QDateTime::fromString(date, Qt::ISODate) }
{
}

VersionInfo::VersionInfo(const QString & sha1, const QString & rev, const QString & date)
    : VersionInfo(-1, -1, -1, QString(), sha1, rev, date)
{
}
