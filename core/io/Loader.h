#pragma once

#include <map>
#include <memory>
#include <type_traits>

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
        CSV,
        PolyData,
        Image2D, 
        Volume,
    };

    /** Provide a list of file extension filters as expected by QFileDialog for all supported file types of the 
      * specified category. If multiple file types are supported, an entry "All Supported Files" is added. */
    static const QString & fileFormatFilters(Category category = Category::all);
    static const std::map<QString, QStringList> & fileFormatExtensions(Category category = Category::all);

    static std::unique_ptr<DataObject> readFile(const QString & filename);

    template<typename T> 
    static typename std::enable_if<std::is_base_of<DataObject, T>::value, std::unique_ptr<T>>::type
    readFile(const QString & fileName);

private:
    static const std::map<Category, std::map<QString, QStringList>> &fileFormatExtensionMaps();
};


#include "Loader.hpp"
