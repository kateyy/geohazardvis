#pragma once

#include <QString>

#include <core/core_api.h>


class DataObject;


class CORE_API Loader
{
public:
    static DataObject * readFile(const QString & filename);
};
