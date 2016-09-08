#pragma once

#include <core/core_api.h>


class QString;


class CORE_API RuntimeInfo
{
public:
    static const QString & dataPath();
    static const QString & pluginsPath();

private:
    RuntimeInfo() = delete;
    ~RuntimeInfo() = delete;
};
