#pragma once


#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <cinttypes>

#include <vtkSmartPointer.h>

#include "common/ebem3d_common.h"


class vtkDataObject;


enum class DatasetType {
    unknown,
    vertices,   // index + vec3
    indices,    // indices referring to a vertex list
    vectors,    // additional vector data per cell
    grid2D,
    vectorGrid3D
};

enum class ModelType
{
    raw,
    triangles,
    DEM,
    grid2D,
    vectorGrid3D
};

struct InputFileInfo
{
    InputFileInfo(const std::string & name, const ModelType type);

    const std::string name;
    const ModelType type;
};

struct ReadDataset {
    DatasetType type;
    std::vector<std::vector<t_FP>> data;
    std::string attributeName;
    vtkSmartPointer<vtkDataObject> vtkMetaData;
};

class TextFileReader
{
public:
    static std::shared_ptr<InputFileInfo> read(const std::string & filename, std::vector<ReadDataset> & readDatasets);

private:
    struct DatasetDef {
        DatasetType type;
        t_UInt nbLines;
        t_UInt nbColumns;
        std::string attributeName;
        vtkSmartPointer<vtkDataObject> vtkMetaData;
    };

    /// read the file header and leave the input stream at a position directly behind the header end
    /// @return a shared pointer to an InputFileInfo object, if the file contains a valid header
    static std::shared_ptr<InputFileInfo> readHeader(std::ifstream & inputStream, std::vector<DatasetDef>& inputDefs);

    static bool readHeader_triangles(std::ifstream & inputStream, std::vector<DatasetDef>& inputDefs);
    static bool readHeader_DEM(std::ifstream & inputStream, std::vector<DatasetDef>& inputDefs);
    static bool readHeader_grid2D(std::ifstream & inputStream, std::vector<DatasetDef>& inputDefs);
    static bool readHeader_vectorGrid3D(std::ifstream & inputStream, std::vector<DatasetDef>& inputDefs);

    static DatasetType checkDataSetType(const std::string & nameString);
};
