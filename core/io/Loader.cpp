#include "Loader.h"

#include <algorithm>
#include <limits>

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


using namespace std;
using namespace io;

namespace
{
const QMap<QString, QStringList> & vtkImageFormats()
{
    static QMap<QString, QStringList> m;
    if (m.isEmpty())
    {
        auto readers = vtkSmartPointer<vtkImageReader2Collection>::New();
        vtkImageReader2Factory::GetRegisteredReaders(readers);
        for (readers->InitTraversal(); auto reader = readers->GetNextItem();)
        {
            QString desc = QString::fromUtf8(reader->GetDescriptiveName());
            QStringList exts = QString::fromUtf8(reader->GetFileExtensions()).split(" ", QString::SkipEmptyParts);
            std::for_each(exts.begin(), exts.end(), [] (QString & ext) { ext.remove('.'); });
            m.insert(desc, exts);
        }
    }
    return m;
}

const QSet<QString> & vtkImageFileExts()
{
    static QSet<QString> exts;
    if (exts.isEmpty())
        for (const QStringList & f_exts : vtkImageFormats().values())
            exts += f_exts.toSet();

    return exts;
}
}


void Loader::initialize()
{
    fileFormatExtensions();
    fileFormatExtensions();
}

const QString & Loader::fileFormatFilters()
{
    static QString f;
    if (f.isEmpty())
    {
        f = "All Supported Files (";
        QSet<QString> allExts;
        for (auto && it : fileFormatExtensions())
            for (auto && ext : it)
                allExts << ext;

        for (auto && ext : allExts)
            f += "*." + ext + " ";

        f += ")";

        for (auto && it = fileFormatExtensions().begin(); it != fileFormatExtensions().end(); ++it)
        {
            f += ";;" + it.key() + " (";
            for (auto && ext : it.value())
                f += "*." + ext + " ";
            f += ")";
        }
    }

    return f;
}

const QMap<QString, QStringList> & Loader::fileFormatExtensions()
{
    static QMap<QString, QStringList> m;
    if (m.isEmpty())
    {
        m = {
            { "Text files", { "txt" } },
            { "VTK XML Image Files", { "vti" } },
            { "VTK XML PolyData Files", { "vtp" } },
            { "Digital Elevation Model", { "dem" } }
        };
        for (auto it = vtkImageFormats().begin(); it != vtkImageFormats().end(); ++it)
            m.insert(it.key(), it.value());
    }
    return m;
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

            double spacing[3];
            image->GetSpacing(spacing);

            for (int i = 0; i < 3; ++i)
            {
                if (spacing[i] > std::numeric_limits<double>::epsilon())
                {
                    continue;
                }

                qDebug() << "Warning: invalid image spacing near 0, resetting to 1. In" << filename;
                spacing[i] = 1;
            }

            image->SetSpacing(spacing);
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
