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
