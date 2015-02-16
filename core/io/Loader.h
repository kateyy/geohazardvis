#pragma once

#include <QMap>
#include <QStringList>

#include <core/core_api.h>


class DataObject;


class CORE_API Loader
{
public:
    static const QString & fileFormatFilters();
    static const QMap<QString, QStringList> & fileFormatExtensions();

    static DataObject * readFile(const QString & filename);

private:
    static DataObject * loadTextFile(const QString & fileName);
};
