#include "io_helper.h"

#include <set>

#include <QRegExp>

#include <vtkImageReader2.h>
#include <vtkImageReader2Collection.h>
#include <vtkImageReader2Factory.h>
#include <vtkSmartPointer.h>


namespace
{

using namespace io;

const std::map<QString, QStringList> & vtkImageFormats()
{
    static const auto _vtkImageFormats = [] () {
        std::map<QString, QStringList> m;

        auto readers = vtkSmartPointer<vtkImageReader2Collection>::New();
        vtkImageReader2Factory::GetRegisteredReaders(readers);
        for (readers->InitTraversal(); auto reader = readers->GetNextItem();)
        {
            QString desc = QString::fromUtf8(reader->GetDescriptiveName());
            QStringList exts = QString::fromUtf8(reader->GetFileExtensions()).split(" ", QString::SkipEmptyParts);
            std::for_each(exts.begin(), exts.end(), [] (QString & ext) { ext.remove('.'); });
            m.emplace(desc, exts);
        }

        return m;
    }();

    return _vtkImageFormats;
}

const std::map<Category, std::map<QString, QStringList>> & fileFormatExtensionMaps()
{
    static const auto _fileFormatExtensionMaps = [] () {
        std::map<Category, std::map<QString, QStringList>> m = {
            { Category::CSV, { { "CSV Files", { "txt", "csv", "tsv" } } } },
            { Category::PolyData, { { "VTK XML PolyData Files", { "vtp" } } } },
            { Category::Image2D, {
                { "VTK XML Image Files", { "vti" } },
                { "Digital Elevation Model", { "dem" } }
            } },
            { Category::Volume, { { "VTK XML Image Files", { "vti" } } } }
        };

        auto && _vtkImageFormats = vtkImageFormats();
        m[Category::VTKImageFormats].insert(_vtkImageFormats.begin(), _vtkImageFormats.end());
        m[Category::Image2D].insert(_vtkImageFormats.begin(), _vtkImageFormats.end());

        auto & _all = m[Category::all];
        for (const auto & perCategory : m)
        {
            _all.insert(perCategory.second.begin(), perCategory.second.end());
        }

        return m;
    }();

    return _fileFormatExtensionMaps;
}

}


namespace io
{

const QString & fileFormatFilters(const Category category)
{
    static const auto _fileFormatFilters = [] () {
        std::map<Category, QString> m;

        const auto & maps = fileFormatExtensionMaps();
        for (auto && categoryMap : maps)
        {
            QString filters;
            if (categoryMap.second.size() > 1)
            {
                filters = "All Supported Files (";
                // remove duplicates, sort alphabetically
                std::set<QString> allExts;
                for (const auto & it : categoryMap.second)
                {
                    for (const auto & ext : it.second)
                    {
                        allExts.insert(ext);
                    }
                }

                for (const auto & ext : allExts)
                {
                    filters += "*." + ext + " ";
                }

                filters.truncate(filters.length() - 1);
                filters += ");;";
            }

            for (auto && it : categoryMap.second)
            {
                filters += it.first + " (";
                for (const auto & ext : it.second)
                {
                    filters += "*." + ext + " ";
                }
                filters.truncate(filters.length() - 1);
                filters += ");;";
            }

            filters.truncate(filters.length() - 2);

            m.emplace(categoryMap.first, filters);
        }

        return m;
    }();

    return _fileFormatFilters.at(category);
}

const std::map<QString, QStringList> & fileFormatExtensions(Category category)
{
    return fileFormatExtensionMaps().at(category);
}

QString normalizeFileName(const QString & fileName, const QString & replaceString)
{
    auto result = fileName;
    result.replace(QRegExp(R"([<>:"/\|?*])"), replaceString);
    return result;
}

}
