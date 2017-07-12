#pragma once

#include <map>

#include <QStringList>

#include <core/core_api.h>


namespace io
{

enum Category
{
    all,
    CSV,
    PolyData,
    Image2D,
    VTKImageFormats, // subset of Image2D that is supported by VTK
    Volume,
};

/**
 * Provide a list of file extension filters as expected by QFileDialog for all supported file types
 * of the specified category. If multiple file types are supported, an entry "All Supported Files"
 * is added.
 */
CORE_API const QString & fileFormatFilters(Category category = Category::all);
CORE_API const std::map<QString, QStringList> & fileFormatExtensions(Category category = Category::all);

/** Replace characters not representable in the current OS's file system logic. */
CORE_API QString normalizeFileName(const QString & fileName, const QString & replaceString = "_");

}
