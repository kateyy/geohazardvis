#include "VersionInfo.h"

#include <QDateTime>
#include <QString>

#include "vcs_commit_info.h"


const QString & VersionInfo::gitSHA1()
{
    static auto sha1_s = QString::fromLatin1(GIT_SHA1);

    return sha1_s;
}

const QString & VersionInfo::gitRevision()
{
    static auto rev_s = QString::fromLatin1(GIT_REV);

    return rev_s;
}

const QString & VersionInfo::gitCommitDateString()
{
    static auto date_s = QString::fromLatin1(GIT_DATE);

    return date_s;
}

const QDateTime & VersionInfo::gitCommitDate()
{
    static auto date_d = QDateTime::fromString(gitCommitDateString(), Qt::DateFormat::ISODate);

    return date_d;
}
