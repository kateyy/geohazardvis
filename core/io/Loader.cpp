#include "Loader.h"

#include <QFileInfo>
#include <QDebug>

#include <vtkImageData.h>
#include <vtkXMLImageDataReader.h>

#include <core/vtkhelper.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/io/TextFileReader.h>
#include <core/io/MatricesToVtk.h>


using namespace std;


DataObject * Loader::readFile(const QString & filename)
{
    QFileInfo fileInfo{ filename };
    QString baseName = fileInfo.baseName();
    QString ext = fileInfo.suffix().toLower();
    
    if (ext == "vti")
    {
        VTK_CREATE(vtkXMLImageDataReader, reader);
        reader->SetFileName(filename.toLatin1().data());
        reader->Update();
        vtkSmartPointer<vtkImageData> image = reader->GetOutput();

        if (!image)
        {
            qDebug() << "Invalid VTK image file: " << filename;
            return nullptr;
        }

        switch (image->GetDataDimension())
        {
        case 2:
            return new ImageDataObject(baseName, image);
        case 3:
            return new VectorGrid3DDataObject(baseName, image);

        default:
            qDebug() << "VTK image data format not supported.";
            return nullptr;
        }
    }

    // handle all other files as our text file format
    return loadTextFile(filename);
}

DataObject * Loader::loadTextFile(const QString & filename)
{
    std::vector<ReadDataset> readDatasets;
    std::shared_ptr<InputFileInfo> inputInfo = TextFileReader::read(filename.toStdString(), readDatasets);
    if (!inputInfo)
        return nullptr;

    QString dataSetName = QString::fromStdString(inputInfo->name);
    switch (inputInfo->type)
    {
    case ModelType::triangles:
        return MatricesToVtk::loadIndexedTriangles(dataSetName, readDatasets);
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
