#pragma once

#include <map>
#include <memory>

#include <core/core_api.h>


class QString;
class QStringList;

class DataObject;


class CORE_API Loader
{
public:
    enum Category
    {
        all,
        ASCII,
        PolyData,
        Image2D, 
        Volume,
    };

    /** Provide a list of file extension filters as expected by QFileDialog for all supported file types of the 
      * specified category. If multiple file types are supported, an entry "All Supported Files" is added. */
    static const QString & fileFormatFilters(Category category = Category::all);
    static const std::map<QString, QStringList> & fileFormatExtensions(Category category = Category::all);

    static std::unique_ptr<DataObject> readFile(const QString & filename);

private:
    static std::unique_ptr<DataObject> loadTextFile(const QString & fileName);

    static const std::map<Category, std::map<QString, QStringList>> &fileFormatExtensionMaps();
};
