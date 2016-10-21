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
