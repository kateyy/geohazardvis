#include "Loader.h"

#include <core/io/TextFileReader.h>
#include <core/io/MatricesToVtk.h>


using namespace std;


DataObject * Loader::readFile(const QString & filename)
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
