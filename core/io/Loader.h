#pragma once

#include <memory>

#include <core/core_api.h>


template<typename K, typename V> class QMap;
class QString;
class QStringList;

class DataObject;


class CORE_API Loader
{
public:
    /** Setup some static data. An explicit initialization is required in a multi-threaded application
        when using the VTK leak debuggers. */
    static void initialize();

    static const QString & fileFormatFilters();
    static const QMap<QString, QStringList> & fileFormatExtensions();

    static std::unique_ptr<DataObject> readFile(const QString & filename);

private:
    static std::unique_ptr<DataObject> loadTextFile(const QString & fileName);
};
