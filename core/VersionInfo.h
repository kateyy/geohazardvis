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
