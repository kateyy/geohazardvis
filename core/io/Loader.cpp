#include "Loader.h"

#include <algorithm>
#include <cassert>
#include <set>

#include <QFileInfo>
#include <QDebug>

#include <vtkCellData.h>
#include <vtkCharArray.h>
#include <vtkDEMReader.h>
#include <vtkExecutive.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkImageReader2.h>
#include <vtkImageReader2Collection.h>
#include <vtkImageReader2Factory.h>
#include <vtkNew.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLPolyDataReader.h>

#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/io/TextFileReader.h>
#include <core/io/types.h>
#include <core/io/MatricesToVtk.h>


using namespace io;

namespace
{
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

const QSet<QString> & vtkImageFileExts()
{
    static const auto _vtkImageFileExts = [] () {
        QSet<QString> exts;
        for (const auto & f_exts : vtkImageFormats())
            exts += f_exts.second.toSet();
        return exts;
    }();

    return _vtkImageFileExts;
}
}

const QString & Loader::fileFormatFilters(Category category)
{
    static const auto _fileFormatFilters = [] () {
        std::map<Category, QString> m;

        const auto & maps = fileFormatExtensionMaps();
        for (auto categoryMap = maps.begin(); categoryMap != maps.end(); ++categoryMap)
        {
            QString filters;
            if (categoryMap->second.size() > 1)
            {
                filters = "All Supported Files (";
                // remove duplicates, sort alphabetically
                std::set<QString> allExts;
                for (const auto & it : categoryMap->second)
                    for (const auto & ext : it.second)
                        allExts.insert(ext);

                for (const auto & ext : allExts)
                    filters += "*." + ext + " ";

                filters.truncate(filters.length() - 1);
                filters += ");;";
            }

            for (auto it = categoryMap->second.begin(); it != categoryMap->second.end(); ++it)
            {
                filters += it->first + " (";
                for (const auto & ext : it->second)
                    filters += "*." + ext + " ";
                filters.truncate(filters.length() - 1);
                filters += ");;";
            }

            filters.truncate(filters.length() - 2);

            m.emplace(categoryMap->first, filters);
        }

        return m;
    }();

    return _fileFormatFilters.at(category);
}

const std::map<QString, QStringList> & Loader::fileFormatExtensions(Category category)
{
    return fileFormatExtensionMaps().at(category);
}

const std::map<Loader::Category, std::map<QString, QStringList>> & Loader::fileFormatExtensionMaps()
{
    static const auto _fileFormatExtensionMaps = [] () {
        std::map<Category, std::map<QString, QStringList>> m = {
            { Category::CSV, { { "CSV Files", { "txt", "csv" } } } },
            { Category::PolyData, { { "VTK XML PolyData Files", { "vtp" } } } },
            { Category::Image2D, {
                { "VTK XML Image Files", { "vti" } },
                { "Digital Elevation Model", { "dem" } }
            } },
            { Category::Volume, { { "VTK XML Image Files", { "vti" } } } }
        };

        auto && _vtkImageFormats = vtkImageFormats();
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

std::unique_ptr<DataObject> Loader::readFile(const QString & filename)
{
    {
        QFile f(filename);
        if (!f.exists())
        {
            qDebug() << "Loader: trying to open non-existing file: " << filename;
            return nullptr;
        }
        if (!f.open(QIODevice::ReadOnly))
        {
            qDebug() << "Loader: cannot open file for read-only access: " << filename;
            return nullptr;
        }
    }

    QFileInfo fileInfo{ filename };
    QString baseName = fileInfo.baseName();
    QString ext = fileInfo.suffix().toLower();

    auto getName = [] (vtkDataSet * dataSet) -> QString {
        vtkCharArray * nameArray = vtkCharArray::SafeDownCast(dataSet->GetFieldData()->GetArray("Name"));
        if (!nameArray)
            return "";

        return QString::fromUtf8(
            nameArray->GetPointer(0),
            static_cast<int>(nameArray->GetSize()));
    };

    if (ext == "vti" || ext == "dem")
    {
        vtkSmartPointer<vtkImageData> image;

        if (ext == "vti")
        {
            vtkNew<vtkXMLImageDataReader> reader;
            reader->SetFileName(filename.toUtf8().data());

            if (reader->GetExecutive()->Update() == 1)
            {
                image = reader->GetOutput();
            }

            if (!image)
            {
                qDebug() << "Invalid VTK image file: " << filename;
                return nullptr;
            }
        }
        else if (ext == "dem")
        {
            vtkNew<vtkDEMReader> reader;
            reader->SetFileName(filename.toUtf8().data());

            if (reader->GetExecutive()->Update() == 1)
                image = reader->GetOutput();

            if (!image)
            {
                qDebug() << "Invalid DEM file: " << filename;
                return nullptr;
            }
        }

        assert(image);

        QString dataSetName = getName(image);
        if (dataSetName.isEmpty())
        {
            vtkDataArray * data = nullptr;
            if (!(data = image->GetPointData()->GetScalars()))
                if (!(data = image->GetPointData()->GetVectors()))
                    if (!(data = image->GetCellData()->GetScalars()))
                        data = image->GetCellData()->GetVectors();
            if (data && data->GetName())
                dataSetName = QString::fromUtf8(data->GetName());
        }
        if (dataSetName.isEmpty())
            dataSetName = baseName;

        switch (image->GetDataDimension())
        {
        case 2:
            return std::make_unique<ImageDataObject>(dataSetName, *image);
        case 3:
            return std::make_unique<VectorGrid3DDataObject>(dataSetName, *image);

        default:
            qDebug() << "VTK image data format not supported.";
            return nullptr;
        }
    }

    if (ext == "vtp")
    {
        auto reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
        reader->SetFileName(filename.toUtf8().data());

        vtkSmartPointer<vtkPolyData> polyData;
        if (reader->GetExecutive()->Update() == 1)
            polyData = reader->GetOutput();

        if (!polyData)
        {
            qDebug() << "Invalid VTK PolyData file: " << filename;
            return nullptr;
        }

        QString dataSetName = getName(polyData);
        if (dataSetName.isEmpty())
        {
            vtkDataArray * data = nullptr;
            if (!(data = polyData->GetCellData()->GetScalars()))
                if (!(data = polyData->GetPointData()->GetScalars()))
                    if (!(data = polyData->GetCellData()->GetVectors()))
                        data = polyData->GetPointData()->GetVectors();
            if (data && data->GetName())
                dataSetName = QString::fromUtf8(data->GetName());
        }
        if (dataSetName.isEmpty())
            dataSetName = baseName;

        return std::make_unique<PolyDataObject>(dataSetName, *polyData);
    }

    if (vtkImageFileExts().contains(ext))
    {
        vtkSmartPointer<vtkImageReader2> reader;
        reader.TakeReference(
            vtkImageReader2Factory::CreateImageReader2(filename.toUtf8().data()));

        if (!reader)
        {
            qDebug() << "Unsupported image format: " << filename;
            return nullptr;
        }

        vtkImageData * image = nullptr;
        reader->SetFileName(filename.toUtf8().data());
        if (reader->GetExecutive()->Update() == 1)
            image = reader->GetOutput();

        if (!image)
        {
            qDebug() << "Invalid image file: " << filename;
            return nullptr;
        }

        vtkDataArray * scalars = image->GetPointData()->GetScalars();
        // readers set the scalar name to something like "JPEGdata"..
        if (scalars)
            scalars->SetName(baseName.toUtf8().data());

        return std::make_unique<ImageDataObject>(baseName, *image);
    }

    // handle all other files as our text file format
    return loadTextFile(filename);
}

std::unique_ptr<DataObject> Loader::loadTextFile(const QString & filename)
{
    std::vector<ReadDataSet> readDatasets;
    std::shared_ptr<InputFileInfo> inputInfo = TextFileReader::read(filename.toStdString(), readDatasets);
    if (!inputInfo)
        return nullptr;

    QString dataSetName = QString::fromStdString(inputInfo->name);
    switch (inputInfo->type)
    {
    case ModelType::triangles:
        return MatricesToVtk::loadIndexedTriangles(dataSetName, readDatasets);
    case ModelType::DEM:
        return MatricesToVtk::loadDEM(dataSetName, readDatasets);
    case ModelType::grid2D:
        return MatricesToVtk::loadGrid2D(dataSetName, readDatasets);
    case ModelType::vectorGrid3D:
        return MatricesToVtk::loadGrid3D(dataSetName, readDatasets);
    case ModelType::raw:
        return MatricesToVtk::readRawFile(filename);
    default:
        cerr << "Warning: model type unsupported by the loader: " << int(inputInfo->type) << endl;
        return nullptr;
    }
}
