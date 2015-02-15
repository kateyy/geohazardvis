#pragma once

#include <QList>
#include <QMap>
#include <QString>

#include <core/core_api.h>


class DataObject;


class CORE_API Loader
{
public:
    static const QString & fileFormatFilters();
    static const QMap<QString, QList<QString>> & fileFormatExtensions();

    static DataObject * readFile(const QString & filename);

private:
    static DataObject * loadTextFile(const QString & fileName);
};
