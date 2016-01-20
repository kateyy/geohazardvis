#pragma once

#include <core/core_api.h>


class QDateTime;
class QString;


class CORE_API VersionInfo
{
public:
    static const QString & gitSHA1();
    static const QString & gitRevision();
    static const QString & gitCommitDateString();
    static const QDateTime & gitCommitDate();
};
