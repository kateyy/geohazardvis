#include "VersionInfo.h"

#include <QDateTime>
#include <QString>

#include "vcs_commit_info.h"


const QString & VersionInfo::maintainerEmail()
{
    static const auto email = QString::fromLatin1(PROJECT_MAINTAINER_EMAIL);

    return email;
}

int VersionInfo::versionMajor()
{
    return PROJECT_VERSION_MAJOR;
}

int VersionInfo::versionMinor()
{
    return PROJECT_VERSION_MINOR;
}

int VersionInfo::versionPatch()
{
    return PROJECT_VERSION_PATCH;
}

const QString & VersionInfo::gitSHA1()
{
    static const auto sha1_s = QString::fromLatin1(GIT_SHA1);

    return sha1_s;
}

const QString & VersionInfo::gitRevision()
{
    static const auto rev_s = QString::fromLatin1(GIT_REV);

    return rev_s;
}

const QString & VersionInfo::gitCommitDateString()
{
    static const auto date_s = QString::fromLatin1(GIT_DATE);

    return date_s;
}

const QDateTime & VersionInfo::gitCommitDate()
{
    static const auto date_d = QDateTime::fromString(gitCommitDateString(), Qt::DateFormat::ISODate);

    return date_d;
}
